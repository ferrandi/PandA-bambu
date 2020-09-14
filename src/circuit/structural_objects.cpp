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
 * @file structural_objects.cpp
 * @brief This class represents a base circuit object
 *
 * @author Matteo Barbati <mbarbati@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#include "structural_objects.hpp"

#include "config_HAVE_ASSERTS.hpp"
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_TECHNOLOGY_BUILT.hpp"
#include "config_HAVE_TUCANO_BUILT.hpp"
#include "config_RELEASE.hpp"

#include "HDL_manager.hpp"                    // for structur...
#include "NP_functionality.hpp"               // for NP_funct...
#include "custom_set.hpp"                     // for set, set...
#include <algorithm>                          // for find, min
#include <boost/algorithm/string/replace.hpp> // for replace_all
#include <boost/iterator/iterator_facade.hpp> // for operator!=
#include <boost/iterator/iterator_traits.hpp> // for iterator...
#include <boost/lexical_cast.hpp>             // for lexical_...
#include <climits>                            // for UINT_MAX
#include <iostream>                           // for cout
#include <list>                               // for _List_co...
#include <memory>                             // for allocato...
#include <utility>
#if HAVE_BAMBU_BUILT
#include "behavioral_helper.hpp" // for Behavior...
#endif
#include "dbgPrintHelper.hpp"     // for DEBUG_LE...
#include "exceptions.hpp"         // for THROW_AS...
#include "library_manager.hpp"    // for attribute
#include "simple_indent.hpp"      // for simple_i...
#include "structural_manager.hpp" // for structur...
#include "technology_manager.hpp" // for technolo...
#include "technology_node.hpp"    // for function...
#include "utility.hpp"            // for GET_CLAS...
#include "xml_attribute.hpp"      // for attribut...
#include "xml_element.hpp"        // for xml_element
#include "xml_helper.hpp"         // for CE_XVM
#include "xml_node.hpp"           // for xml_node...
#include "xml_text_node.hpp"      // for xml_text...

/// utility include
#include "string_manipulation.hpp"

#if HAVE_EXPERIMENTAL
#include "layout_model.hpp"
#endif

inline std::string legalize(std::string& id)
{
   // boost::replace_all(id, "[", "_");
   // boost::replace_all(id, "]", "_");
   // boost::replace_all(id, "\\", "s_");
   return id;
}

const char* structural_type_descriptor::s_typeNames[] = {"OTHER", "BOOL", "INT", "UINT", "REAL", "USER", "VECTOR_BOOL", "VECTOR_INT", "VECTOR_UINT", "VECTOR_REAL", "VECTOR_USER", "UNKNOWN"};

const std::string structural_type_descriptor::get_name() const
{
   for(int i = 0; i < UNKNOWN; i++)
      if(type == i)
         return boost::lexical_cast<std::string>(s_typeNames[i]);
   return "UNKNOWN";
}

void structural_type_descriptor::xload(const xml_element* Enode, structural_type_descriptorRef)
{
   if(CE_XVM(type, Enode))
   {
      unsigned int i;
      std::string type_string;
      LOAD_XVFM(type_string, Enode, type);
      for(i = 0; i < UNKNOWN; i++)
         if(type_string == s_typeNames[i])
            break;
      this->type = s_type(i);
   }
   else
      type = type_DEFAULT;
   if(type == BOOL)
      size = 1;
   if(CE_XVM(size, Enode))
      LOAD_XVM(size, Enode);
   if(CE_XVM(vector_size, Enode))
      LOAD_XVM(vector_size, Enode);
   if(CE_XVM(id_type, Enode))
      LOAD_XVM(id_type, Enode);
   if(CE_XVM(treenode, Enode))
      LOAD_XVM(treenode, Enode);
   THROW_ASSERT(type != type_DEFAULT or id_type.size(), "Wrong type descriptor");
}

void structural_type_descriptor::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   if(type != type_DEFAULT)
      WRITE_XNVM(type, s_typeNames[type], Enode);
   if(size != size_DEFAULT)
      WRITE_XVM(size, Enode);
   if(vector_size != size_DEFAULT)
      WRITE_XVM(vector_size, Enode);
   if(id_type != "")
      WRITE_XVM(id_type, Enode);
   if(treenode != treenode_DEFAULT)
      WRITE_XVM(treenode, Enode);
}

void structural_type_descriptor::copy(structural_type_descriptorRef dest)
{
   dest->id_type = id_type;
   dest->type = type;
   dest->size = size;
   dest->vector_size = vector_size;
   dest->treenode = treenode;
}

void structural_type_descriptor::print(std::ostream& os) const
{
   switch(type)
   {
      case BOOL:
      {
         THROW_ASSERT(size == 1 && vector_size == 0, "bool type descriptor not correctly defined" + STR(size) + "|" + STR(vector_size));
         os << "Bool {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         break;
      }
      case INT:
      {
         THROW_ASSERT(size > 0 && vector_size == 0, "int type descriptor not correctly defined");
         os << "Int {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         os << "size=" << size << " ";
         break;
      }
      case UINT:
      {
         THROW_ASSERT(size > 0 && vector_size == 0, "unsigned int type descriptor not correctly defined");
         os << "Unsigned Int {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         os << "size=" << size << " ";
         break;
      }
      case REAL:
      {
         THROW_ASSERT((size > 0 && vector_size == 0) || (size == 1 && (vector_size == 32 || vector_size == 64 || vector_size == 96 || vector_size == 128)), "real type descriptor not correctly defined " + STR(size) + " " + STR(vector_size));
         os << "Real {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         os << "size=" << size << " ";
         break;
      }
      case USER:
      {
         os << "User type {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         os << "size=" << size << " ";
         break;
      }
      case VECTOR_BOOL:
      {
         THROW_ASSERT(size == 1 && vector_size > 0, "bool vector type descriptor not correctly defined");
         os << "Bool Vector [" << vector_size << "] {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         break;
      }
      case VECTOR_INT:
      {
         THROW_ASSERT(size > 0 && vector_size > 0, "int vector type descriptor not correctly defined");
         os << "Int Vector [" << vector_size << "] {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         os << "size=" << size << " ";
         break;
      }
      case VECTOR_UINT:
      {
         THROW_ASSERT(size > 0 && vector_size > 0, "unsigned int vector type descriptor not correctly defined");
         os << "Unsigned Int Vector [" << vector_size << "] {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         os << "size=" << size << " ";
         break;
      }
      case VECTOR_REAL:
      {
         THROW_ASSERT(size > 0 && vector_size > 0, "real vector type descriptor not correctly defined");
         os << "Real Vector [" << vector_size << "] {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         os << "size=" << size << " ";
         break;
      }
      case VECTOR_USER:
      {
         THROW_ASSERT(vector_size > 0, "Vector User type descriptor not correctly defined");
         os << "User Vector type [" << vector_size << "] {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         os << "size=" << size << " ";
         break;
      }
      case OTHER:
         os << "Other type {" << id_type << "} ";
         if(treenode > 0)
            os << "(@" << treenode << ") ";
         break;
      case UNKNOWN:
      default:
         THROW_ERROR("Not initialized type");
   }
}

structural_type_descriptor::structural_type_descriptor(const std::string& type_name, unsigned int _vector_size) : vector_size(_vector_size), id_type(type_name), treenode(structural_type_descriptor::treenode_DEFAULT)
{
   /// first set defaults
   type = UNKNOWN;
   size = size_DEFAULT;
   if(_vector_size == 0)
   {
      if(type_name == "bool")
      {
         type = BOOL;
         size = 1;
      }
      else if(type_name == "int")
      {
         type = INT;
         size = 32;
      }
      else if(type_name == "unsigned int")
      {
         type = UINT;
         size = 32;
      }
      else if(type_name == "float")
      {
         type = REAL;
         size = 32;
      }
      else if(type_name == "double")
      {
         type = REAL;
         size = 64;
      }
      else if(type_name == "long double")
      {
         type = REAL;
         size = 96;
      }
      else if(type_name == "long long double")
      {
         type = REAL;
         size = 128;
      }
      else
         THROW_ERROR("not supported type name");
   }
   else
   {
      if(type_name == "bool")
      {
         type = VECTOR_BOOL;
         size = 1;
      }
      else if(type_name == "int")
      {
         type = VECTOR_INT;
         size = 32;
      }
      else if(type_name == "unsigned int")
      {
         type = VECTOR_UINT;
         size = 32;
      }
      else if(type_name == "float")
      {
         type = VECTOR_REAL;
         size = 32;
      }
      else if(type_name == "double")
      {
         type = VECTOR_REAL;
         size = 64;
      }
      else if(type_name == "long double")
      {
         type = VECTOR_REAL;
         size = 96;
      }
      else if(type_name == "long long double")
      {
         type = VECTOR_REAL;
         size = 128;
      }
      else
         THROW_ERROR("not supported type name");
   }
}

#if HAVE_TUCANO_BUILT
structural_type_descriptor::structural_type_descriptor(unsigned int _treenode, tree_managerRef tm)
{
   bool is_a_function = false;
   /// first set defaults
   type = UNKNOWN;
   size = size_DEFAULT;
   vector_size = vector_size_DEFAULT;
   treenode = _treenode;
   while(true)
   {
      if(tree_helper::GetElements(tm, treenode))
      {
         treenode = tree_helper::GetElements(tm, treenode);
         continue;
      }
      if(tree_helper::get_pointed_type(tm, treenode))
      {
         treenode = tree_helper::get_pointed_type(tm, treenode);
         continue;
      }
      if(tree_helper::is_a_function(tm, treenode))
      {
         is_a_function = true;
         continue;
      }
      break;
   }
   vector_size = tree_helper::size(tm, treenode);

   if(is_a_function || tree_helper::is_module(tm, treenode) || tree_helper::is_channel(tm, treenode) || tree_helper::is_event(tm, treenode))
   {
      id_type = tree_helper::name_type(tm, treenode);
      type = OTHER;
   }
   else
   {
      id_type = tree_helper::name_type(tm, treenode);
      size = tree_helper::size(tm, treenode);
      if(tree_helper::is_bool(tm, treenode))
      {
         if(vector_size)
            type = VECTOR_BOOL;
         else
            type = BOOL;
      }
      else if(tree_helper::is_int(tm, treenode))
      {
         if(vector_size)
            type = VECTOR_INT;
         else
            type = INT;
      }
      else if(tree_helper::is_unsigned(tm, treenode))
      {
         if(vector_size)
            type = VECTOR_UINT;
         else
            type = UINT;
      }
      else
      {
         if(vector_size)
            type = VECTOR_USER;
         else
            type = USER;
      }
   }
}
#endif

#if HAVE_BAMBU_BUILT
structural_type_descriptor::structural_type_descriptor(unsigned int index, const BehavioralHelperConstRef helper)
{
   unsigned int type_index = helper->get_type(index);
   /// first set defaults5
   type = UNKNOWN;
   size = size_DEFAULT;
   vector_size = vector_size_DEFAULT;
   treenode = type_index;
   const unsigned int unqualified_type = helper->GetUnqualified(type_index);
   if(unqualified_type == 0)
   {
      id_type = helper->print_type(type_index);
   }
   else
   {
      id_type = helper->print_type(unqualified_type);
   }
   size = helper->get_size(index);
   vector_size = 0;
   if(helper->is_a_pointer(index))
   {
      type = VECTOR_BOOL;
      vector_size = size;
      size = 1;
   }
   else if(helper->is_an_array(index))
   {
      const unsigned int element_type = helper->GetElements(type_index);
      const auto element_size = static_cast<unsigned int>(helper->get_size(element_type));
      vector_size = size / element_size;
      size = element_size;
      if(helper->is_bool(element_type) || helper->is_a_complex(index))
         type = VECTOR_BOOL;
      else if(helper->is_int(element_type))
         type = VECTOR_INT;
      else if(helper->is_unsigned(element_type))
         type = VECTOR_UINT;
      else if(helper->is_real(element_type))
         type = VECTOR_REAL;
      else
      {
         THROW_ERROR("vector user type not supported");
         type = VECTOR_USER;
      }
   }
   else if(helper->is_a_vector(index))
   {
      const unsigned int element_type = helper->GetElements(type_index);
      const auto element_size = static_cast<unsigned int>(helper->get_size(element_type));
      vector_size = size / element_size;
      size = element_size;
      if(helper->is_bool(element_type) || helper->is_a_complex(index))
         type = VECTOR_BOOL;
      else if(helper->is_int(element_type))
         type = VECTOR_INT;
      else if(helper->is_unsigned(element_type))
         type = VECTOR_UINT;
      else if(helper->is_real(element_type))
         type = VECTOR_REAL;
      else
      {
         THROW_ERROR("vector user type not supported");
         type = VECTOR_USER;
      }
   }
   else
   {
      if(helper->is_bool(index))
      {
         type = VECTOR_BOOL;
         size = 1;
         vector_size = 1;
      }
      else if(helper->is_int(index))
         type = INT;
      else if(helper->is_unsigned(index))
         type = UINT;
      else if(helper->is_real(index))
         type = REAL;
      else if(helper->is_a_complex(index))
      {
         type = VECTOR_BOOL;
         vector_size = size;
         size = 1;
      }
      else
      {
         THROW_ERROR("user type not supported: " + STR(index) + "-" + id_type);
         type = USER;
      }
   }
}
#endif

bool structural_type_descriptor::check_type(structural_type_descriptorRef src_type, structural_type_descriptorRef dest_type)
{
   if((src_type->type == dest_type->type && src_type->type != USER && src_type->type != UNKNOWN) || (src_type->type == BOOL && (dest_type->type == INT || dest_type->type == UINT || (dest_type->type == VECTOR_BOOL && dest_type->vector_size == 1))) ||
      (dest_type->type == BOOL && (src_type->type == INT || src_type->type == UINT || (src_type->type == VECTOR_BOOL && src_type->vector_size == 1))) ||
      (src_type->type == VECTOR_BOOL && (dest_type->type == INT || dest_type->type == UINT || dest_type->type == REAL)) || (dest_type->type == VECTOR_BOOL && (src_type->type == INT || src_type->type == UINT || src_type->type == REAL)) ||
      (src_type->type == VECTOR_BOOL && (dest_type->type == VECTOR_INT || dest_type->type == VECTOR_UINT) && (src_type->size * src_type->vector_size == dest_type->size * dest_type->vector_size)) ||
      (dest_type->type == VECTOR_BOOL && (src_type->type == VECTOR_INT || src_type->type == VECTOR_UINT) && (src_type->size * src_type->vector_size == dest_type->size * dest_type->vector_size)) ||
      (src_type->id_type == dest_type->id_type && src_type->id_type != "") || (src_type->treenode == dest_type->treenode && src_type->type > 0) ||
#ifndef NDEBUG
      // Add some SystemC specialization
      (src_type->id_type.find("tlm_fifo<") != std::string::npos && (dest_type->id_type.find("tlm_blocking_put_if<") != std::string::npos || dest_type->id_type.find("tlm_blocking_get_if<") != std::string::npos)) ||
      (dest_type->id_type.find("tlm_fifo<") != std::string::npos && (src_type->id_type.find("tlm_blocking_put_if<") != std::string::npos || src_type->id_type.find("tlm_blocking_get_if<") != std::string::npos))
#else
      // compatibility verified by the gcc compiler!
      (src_type->treenode != treenode_DEFAULT && dest_type->treenode != treenode_DEFAULT)
#endif
   )
      return true;
   else
   {
      src_type->print(std::cout);
      dest_type->print(std::cout);
      THROW_WARNING(std::string("Different types are used " + src_type->id_type + " -- " + dest_type->id_type));
      return false;
   }
}

simple_indent structural_object::PP('[', ']', 2);

/// ------------- structural object methods --------------------- //

structural_object::structural_object(int debug, const structural_objectRef o) : owner(o), treenode(o ? o->treenode : treenode_DEFAULT), black_box(o ? o->black_box : black_box_DEFAULT), debug_level(debug)
{
}

const structural_objectRef structural_object::get_owner() const
{
   return owner.lock();
}

void structural_object::set_owner(const structural_objectRef new_owner)
{
   owner = new_owner;
}

#if HAVE_TECHNOLOGY_BUILT
void structural_object::add_attribute(const std::string& name, const attributeRef& attribute)
{
   if(std::find(attribute_list.begin(), attribute_list.end(), name) == attribute_list.end())
      attribute_list.push_back(name);
   attributes[name] = attribute;
}

attributeRef structural_object::get_attribute(const std::string& name) const
{
   THROW_ASSERT(attributes.find(name) != attributes.end(), "attribute " + name + " does not exist");
   return attributes.find(name)->second;
}

const std::vector<std::string>& structural_object::get_attribute_list() const
{
   return attribute_list;
}
#endif

void structural_object::set_treenode(unsigned int n)
{
   treenode = n;
}

unsigned int structural_object::get_treenode() const
{
   return treenode;
}

void structural_object::set_id(const std::string& s)
{
   id = s;
}

const std::string structural_object::get_id() const
{
   return id;
}

void structural_object::set_type(const structural_type_descriptorRef& s)
{
   type = s;
}

const structural_type_descriptorRef& structural_object::get_typeRef() const
{
   THROW_ASSERT(type, "Structural type descriptor not available for " + get_id());
   return type;
}

void structural_object::type_resize(unsigned int new_bit_size)
{
   switch(type->type)
   {
      case structural_type_descriptor::INT:
      case structural_type_descriptor::UINT:
      case structural_type_descriptor::REAL:
      {
         if(type->size < new_bit_size)
            type->size = new_bit_size;
         break;
      }
      case structural_type_descriptor::VECTOR_BOOL:
      {
         if(type->vector_size < new_bit_size)
            type->vector_size = new_bit_size;
         break;
      }
      case structural_type_descriptor::BOOL:
      {
         THROW_ASSERT(new_bit_size == 1, "BOOL only supports single bit values: " + boost::lexical_cast<std::string>(new_bit_size) + " - " + get_path());
         type->size = new_bit_size;
         break;
      }
      case structural_type_descriptor::USER:
      case structural_type_descriptor::VECTOR_INT:
      case structural_type_descriptor::VECTOR_UINT:
      case structural_type_descriptor::VECTOR_REAL:
      case structural_type_descriptor::VECTOR_USER:
      case structural_type_descriptor::OTHER:
      case structural_type_descriptor::UNKNOWN:
      default:
         THROW_ERROR("Not correct resizing  " + get_path() + " (" + type->id_type + ") New size " + boost::lexical_cast<std::string>(new_bit_size));
   }
}

void structural_object::type_resize(unsigned int new_bit_size, unsigned int new_vec_size)
{
   switch(type->type)
   {
      case structural_type_descriptor::VECTOR_INT:
      case structural_type_descriptor::VECTOR_UINT:
      case structural_type_descriptor::VECTOR_REAL:
      {
         if(type->size < new_bit_size)
            type->size = new_bit_size;
         if(type->vector_size < new_vec_size)
            type->vector_size = new_vec_size;
         break;
      }
      case structural_type_descriptor::VECTOR_BOOL:
      {
         if(type->vector_size < new_bit_size * new_vec_size)
            type->vector_size = new_bit_size * new_vec_size;
         break;
      }
      case structural_type_descriptor::INT:
      case structural_type_descriptor::UINT:
      {
         if(type->size < new_bit_size * new_vec_size)
            type->size = new_bit_size * new_vec_size;
         break;
      }
      case structural_type_descriptor::BOOL:
      case structural_type_descriptor::REAL:
      case structural_type_descriptor::USER:
      case structural_type_descriptor::VECTOR_USER:
      case structural_type_descriptor::OTHER:
      case structural_type_descriptor::UNKNOWN:
      default:
         THROW_ERROR("Not correct resizing " + get_path() + " (" + type->id_type + ") New size " + boost::lexical_cast<std::string>(new_bit_size) + "(" + STR(new_vec_size) + ")");
   }
}

void structural_object::copy(structural_objectRef dest) const
{
   /// the owner has to be already set.
   dest->id = id;
   dest->type = structural_type_descriptorRef(new structural_type_descriptor);
   type->copy(dest->type);
   dest->treenode = treenode;
   dest->black_box = black_box;
   dest->debug_level = debug_level;
   dest->default_parameters = default_parameters;
   dest->parameters = parameters;

#if HAVE_TECHNOLOGY_BUILT
   dest->attribute_list = attribute_list;
   dest->attributes = attributes;
#endif
}

void structural_object::set_black_box(bool bb)
{
   black_box = bb;
}

bool structural_object::get_black_box() const
{
   return black_box;
}

void structural_object::SetParameter(const std::string& name, const std::string& value)
{
   THROW_ASSERT(default_parameters.find(name) != default_parameters.end(), "Parameter " + name + " does not exist in " + get_typeRef()->id_type);
   parameters[name] = value;
}

std::string structural_object::GetParameter(std::string name) const
{
   if(parameters.find(name) != parameters.end())
   {
      return parameters.at(name);
   }
   THROW_ASSERT(default_parameters.find(name) != default_parameters.end(), "Parameter " + name + " has no value associated for unit " + get_typeRef()->id_type);
   return default_parameters.at(name);
}

void structural_object::AddParameter(const std::string& name, const std::string& default_value)
{
   THROW_ASSERT(default_parameters.find(name) == default_parameters.end() or default_parameters.at(name) == default_value, "Parameter " + name + " already added. Old default: " + default_parameters.at(name) + " New default: " + default_value);
   default_parameters[name] = default_value;
}

std::string structural_object::GetDefaultParameter(std::string name) const
{
   THROW_ASSERT(default_parameters.find(name) != default_parameters.end(), "Parameter " + name + " does not exist");
   return default_parameters.at(name);
}

CustomMap<std::string, std::string> structural_object::GetParameters()
{
   CustomMap<std::string, std::string> ret;
   for(const auto& default_parameter : default_parameters)
   {
      ret[default_parameter.first] = default_parameter.second;
   }
   for(const auto& parameter : parameters)
   {
      ret[parameter.first] = parameter.second;
   }
   return ret;
}

#if HAVE_TECHNOLOGY_BUILT
structural_objectRef module::get_generic_object(const technology_managerConstRef TM) const
{
   const auto module_type = get_typeRef()->id_type;
   technology_nodeRef tn = TM->get_fu(module_type, TM->get_library(module_type));
   if(tn->get_kind() == functional_unit_K)
      return GetPointer<functional_unit>(tn)->CM->get_circ();
   else if(tn->get_kind() == functional_unit_template_K && GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU))
      return GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->CM->get_circ();
   else
      THROW_UNREACHABLE("Unexpected pattern");
   return structural_objectRef();
}

structural_type_descriptor::s_type module::get_parameter_type(const technology_managerConstRef TM, const std::string& name) const
{
   const auto module_type = get_generic_object(TM);
   const auto default_value = module_type->GetDefaultParameter(name);
   if(default_value.substr(0, 2) == "\"\"" and default_value.substr(default_value.size() - 2, 2) == "\"\"")
   {
      return structural_type_descriptor::OTHER;
   }
   if(default_value.front() == '\"' and default_value.back() == '\"')
   {
      const auto content_string = default_value.substr(1, default_value.size() - 2);
      for(const auto character : content_string)
      {
         if(character != '0' and character != '1')
         {
            return structural_type_descriptor::UNKNOWN;
         }
      }
      return structural_type_descriptor::VECTOR_BOOL;
   }
   if(default_value.front() >= '0' and default_value.front() <= '9')
      return structural_type_descriptor::INT;
   if(default_value == "-1")
      return structural_type_descriptor::INT;
   THROW_UNREACHABLE("Value of " + name + " is " + default_value);
   if(get_owner() and GetPointer<const module>(get_owner()))
      return GetPointer<const module>(GetPointer<const module>(get_owner())->get_generic_object(TM))->get_parameter_type(TM, name);
   return structural_type_descriptor::UNKNOWN;
}
#endif

bool structural_object::ExistsParameter(std::string name) const
{
   return default_parameters.find(name) != default_parameters.end();
}

void structural_object::xload(const xml_element* Enode, structural_objectRef, structural_managerRef const&)
{
   /// owner not managed by xload
   if(CE_XVM(id, Enode))
      LOAD_XVM(id, Enode);
   if(CE_XVM(treenode, Enode))
      LOAD_XVM(treenode, Enode);
   if(CE_XVM(black_box, Enode))
      LOAD_XVM(black_box, Enode);
      // Recourse through child nodes
#if HAVE_ASSERTS
   bool has_structural_type_descriptor = false;
#endif
   const xml_node::node_list list = Enode->get_children();
   for(const auto& iter : list)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == GET_CLASS_NAME(structural_type_descriptor))
      {
         type = structural_type_descriptorRef(new structural_type_descriptor);
         type->xload(EnodeC, type);
#if HAVE_ASSERTS
         has_structural_type_descriptor = true;
#endif
      }
      else if(EnodeC->get_name() == "parameter")
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Parameter specification");
         std::string name;
         LOAD_XVM(name, EnodeC);
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_ERROR("parameter definition is missing");
         std::string default_value = text->get_content();
         xml_node::convert_escaped(default_value);
         default_parameters[name] = default_value;
      }
   }
   THROW_ASSERT(has_structural_type_descriptor, "A structural object has to have a type." + boost::lexical_cast<std::string>(Enode->get_line()));
}

void structural_object::xwrite(xml_element* Enode)
{
   WRITE_XVM(id, Enode);
   std::string path = get_path();
   WRITE_XVM(path, Enode);
   if(treenode != treenode_DEFAULT)
      WRITE_XVM(treenode, Enode);
   if(black_box != black_box_DEFAULT)
      WRITE_XVM(black_box, Enode);
   if(type)
      type->xwrite(Enode);
   if(!default_parameters.empty())
   {
      for(const auto& default_parameter : default_parameters)
      {
         xml_element* Enode_parameter = Enode->add_child_element("parameter");
         WRITE_XNVM2("name", default_parameter.first, Enode_parameter);
         Enode_parameter->add_child_text(STR(default_parameter.second));
      }
   }
}

#if HAVE_TECHNOLOGY_BUILT
void structural_object::xwrite_attributes(xml_element*, const technology_nodeRef&)
{
}
#endif

void structural_object::print(std::ostream& os) const
{
   os << "SO: " << id << " {" << type << "}";
   if(treenode > 0)
      os << " (@" << treenode << ")";
   os << (black_box ? " (BLACK BOX)" : "");
   os << " Path: " << get_path();
   PP(os, "\n");
}

const std::string structural_object::get_path() const
{
   if(!get_owner())
      return get_id();
   else
      return get_owner()->get_path() + HIERARCHY_SEPARATOR + get_id();
}

const char* port_o::port_directionNames[] = {"IN", "OUT", "IO", "GEN", "UNKNOWN"};

const char* port_o::port_interfaceNames[] = {"PI_DEFAULT", "PI_RNONE", "PI_WNONE"};

port_o::port_o(int _debug_level, const structural_objectRef o, port_direction _dir, so_kind _port_type)
    : structural_object(_debug_level, o),
      dir(_dir),
      end(NONE),
      pi(port_interface::PI_DEFAULT),
      aligment(port_interface_alignment_DEFAULT),
      is_var_args(is_var_args_DEFAULT),
      is_clock(is_clock_DEFAULT),
      is_extern(is_extern_DEFAULT),
      is_global(is_global_DEFAULT),
      is_reverse(is_reverse_DEFAULT),
      is_memory(is_memory_DEFAULT),
      is_slave(is_slave_DEFAULT),
      is_master(is_master_DEFAULT),
      is_data_bus(is_data_bus_DEFAULT),
      is_addr_bus(is_addr_bus_DEFAULT),
      is_size_bus(is_size_bus_DEFAULT),
      is_tag_bus(is_tag_bus_DEFAULT),
      is_doubled(is_doubled_DEFAULT),
      is_halved(is_halved_DEFAULT),
      is_critical(is_critical_DEFAULT),
      lsb(0),
      port_type(_port_type)
{
#if HAVE_TECHNOLOGY_BUILT
   std::string direction;
   if(_dir == IN)
      direction = "input";
   else
      direction = "output";
   attributeRef dir_attribute(new attribute(attribute::STRING, direction));
   add_attribute("direction", dir_attribute);
#endif
}

void port_o::add_connection(structural_objectRef s)
{
   THROW_ASSERT(s, get_path() + ": NULL object received: " + s->get_path());
   THROW_ASSERT((get_kind() == port_o_K && (s->get_kind() == port_o_K || s->get_kind() == signal_o_K || s->get_kind() == constant_o_K)) || (get_kind() == port_vector_o_K && (s->get_kind() == port_vector_o_K || s->get_kind() == signal_vector_o_K)),
                get_path() + ": port cannot be connected to an object of type: " + std::string(s->get_kind_text()));
   for(auto& connected_object : connected_objects)
   {
      if(connected_object.lock() == s)
         return;
      THROW_ASSERT(!(((s->get_kind() == signal_o_K and connected_object.lock()->get_kind() == signal_o_K) || (s->get_kind() == signal_vector_o_K and connected_object.lock()->get_kind() == signal_vector_o_K)) and
                     s->get_owner() == connected_object.lock()->get_owner()),
                   "The port " + get_path() + " can have only one signal. " + s->get_path() + " and " + connected_object.lock()->get_path());
   }
   connected_objects.push_back(s);
}

void port_o::remove_connection(structural_objectRef s)
{
   THROW_ASSERT(s, get_path() + ": NULL object received");
   auto del = connected_objects.begin();
   for(; del != connected_objects.end(); ++del)
   {
      if((*del).lock() == s)
         break;
   }
   if(del != connected_objects.end())
   {
      connected_objects.erase(del);
   }
}

bool port_o::is_connected(structural_objectRef s) const
{
   THROW_ASSERT(s, "NULL object received");
   if(connected_objects.size() == 0)
      return false;
   auto del = connected_objects.begin();
   for(; del != connected_objects.end(); ++del)
   {
      if((*del).lock() == s)
         return true;
   }
   return false;
}

structural_objectRef port_o::get_connected_signal() const
{
   structural_objectRef sigObj;
   for(const auto& connected_object : connected_objects)
   {
      if(connected_object.lock()->get_kind() == signal_o_K || connected_object.lock()->get_kind() == signal_vector_o_K)
      {
         THROW_ASSERT(!sigObj or sigObj->get_owner() != connected_object.lock()->get_owner(), "Multiple signal connected to the same port: " + get_path());
         sigObj = connected_object.lock();
      }
   }
   return sigObj;
}

void port_o::substitute_connection(structural_objectRef old_conn, structural_objectRef new_conn)
{
   CustomOrderedSet<std::vector<Wrefcount<structural_object>>::iterator> removed;
   bool existing = false;
   for(auto del = connected_objects.begin(); del != connected_objects.end();)
   {
      auto del_curr = del;
      ++del;
      if(del_curr->lock() == new_conn)
         existing = true;
      else if(del_curr->lock() == old_conn)
         removed.insert(del_curr);
      else if(del_curr->lock()->get_kind() == signal_o_K and new_conn->get_kind() == signal_o_K)
         THROW_ERROR("Multiple signals for object: " + get_path());
   }
   if(existing)
   {
      auto deli = removed.begin();
      for(; deli != removed.end(); ++deli)
         connected_objects.erase(*deli);
   }
   else
   {
      for(auto& connected_object : connected_objects)
      {
         if(connected_object.lock() == old_conn)
            connected_object = new_conn;
      }
   }
}

const structural_objectRef port_o::get_connection(unsigned int n) const
{
   THROW_ASSERT(n < connected_objects.size(), "index out of range");
   return connected_objects[n].lock();
}

unsigned int port_o::get_connections_size() const
{
   return static_cast<unsigned int>(connected_objects.size());
}

port_o::port_direction port_o::get_port_direction() const
{
   return dir;
}

void port_o::set_port_direction(port_direction _dir)
{
   dir = _dir;
   for(auto p : ports)
      GetPointer<port_o>(p)->set_port_direction(_dir);
}

port_o::port_endianess port_o::get_port_endianess() const
{
   return end;
}

void port_o::set_port_endianess(port_endianess _end)
{
   end = _end;
}

port_o::port_interface port_o::get_port_interface() const
{
   return pi;
}

void port_o::set_port_interface(port_interface _pi)
{
   pi = _pi;
}

unsigned port_o::get_port_alignment() const
{
   return aligment;
}

void port_o::set_port_alignment(unsigned algn)
{
   aligment = algn;
}

void port_o::set_is_var_args(bool c)
{
   is_var_args = c;
}

bool port_o::get_is_var_args() const
{
   return is_var_args;
}

void port_o::set_is_clock(bool c)
{
   is_clock = c;
}

bool port_o::get_is_clock() const
{
   return is_clock;
}

void port_o::set_is_extern(bool c)
{
   is_extern = c;
}

bool port_o::get_is_extern() const
{
   return is_extern;
}

void port_o::set_bus_bundle(const std::string& name)
{
   bus_bundle = name;
}

std::string port_o::get_bus_bundle() const
{
   return bus_bundle;
}

void port_o::set_is_global(bool c)
{
   is_global = c;
}

bool port_o::get_is_global() const
{
   return is_global;
}

void port_o::set_is_memory(bool c)
{
   is_memory = c;
}

bool port_o::get_is_memory() const
{
   return is_memory;
}

void port_o::set_is_slave(bool c)
{
   is_slave = c;
}

bool port_o::get_is_slave() const
{
   return is_slave;
}

void port_o::set_is_master(bool c)
{
   is_master = c;
}

bool port_o::get_is_master() const
{
   return is_master;
}

void port_o::set_is_data_bus(bool c)
{
   is_data_bus = c;
}

bool port_o::get_is_data_bus() const
{
   return is_data_bus;
}

void port_o::set_is_addr_bus(bool c)
{
   is_addr_bus = c;
}

bool port_o::get_is_addr_bus() const
{
   return is_addr_bus;
}

void port_o::set_is_size_bus(bool c)
{
   is_size_bus = c;
}

bool port_o::get_is_size_bus() const
{
   return is_size_bus;
}

void port_o::set_is_tag_bus(bool c)
{
   is_tag_bus = c;
}

bool port_o::get_is_tag_bus() const
{
   return is_tag_bus;
}

void port_o::set_is_doubled(bool c)
{
   is_doubled = c;
}

bool port_o::get_is_doubled() const
{
   return is_doubled;
}

void port_o::set_is_halved(bool c)
{
   is_halved = c;
}

bool port_o::get_is_halved() const
{
   return is_halved;
}

structural_objectRef port_o::find_bounded_object(const structural_objectConstRef f_owner) const
{
   THROW_ASSERT(get_owner(), "The port has to have an owner " + get_id());
   // THROW_ASSERT(get_owner()->get_owner(), "The owner of the port has to have an owner " + get_id());
   THROW_ASSERT(get_owner()->get_kind() != port_vector_o_K || get_owner()->get_owner(), "The owner of the port_vector has to have an owner " + get_id());
   THROW_ASSERT(get_kind() == port_o_K || get_kind() == port_vector_o_K, "Expected a port got something of different");
   structural_objectRef res;
   unsigned int port_count = 0;
   structural_objectRef _owner;
   if(get_owner()->get_kind() == port_vector_o_K)
      _owner = get_owner()->get_owner();
   else
      _owner = get_owner();

   for(const auto& connected_object : connected_objects)
   {
      if(f_owner)
      {
         if(connected_object.lock()->get_kind() == port_o_K || connected_object.lock()->get_kind() == signal_o_K || connected_object.lock()->get_kind() == constant_o_K)
         {
            if(connected_object.lock()->get_owner()->get_kind() == port_vector_o_K and connected_object.lock()->get_owner()->get_owner() != f_owner)
            {
               continue;
            }
            if((connected_object.lock()->get_owner()->get_kind() != port_vector_o_K and connected_object.lock()->get_owner() != f_owner))
            {
               continue;
            }
         }
         else if(connected_object.lock()->get_owner()->get_kind() == port_vector_o_K || connected_object.lock()->get_owner()->get_kind() == signal_vector_o_K)
         {
            if(connected_object.lock()->get_owner()->get_owner() != f_owner)
               continue;
         }
      }
      THROW_ASSERT(connected_object.lock(), "");

      if(connected_object.lock()->get_owner() == _owner->get_owner())
      {
         res = connected_object.lock();
         port_count++;
      }
      else if((connected_object.lock()->get_owner()->get_kind() == port_vector_o_K || connected_object.lock()->get_owner()->get_kind() == signal_vector_o_K) and
              (connected_object.lock()->get_owner()->get_owner() == _owner->get_owner() || connected_object.lock()->get_owner()->get_owner() == _owner->get_owner()->get_owner()))
      {
         res = connected_object.lock();
         port_count++;
      }
      else if(connected_object.lock()->get_kind() == constant_o_K)
      {
         res = connected_object.lock();
         port_count++;
      }
   }
   if(!port_count)
      return res;

   if(port_count > 1)
   {
      INDENT_DBG_MEX(0, 0, "Too many bindings to " + get_path());
#ifndef NDEBUG
      for(const auto& connected_object : connected_objects)
      {
         INDENT_DBG_MEX(0, 0, "---" + connected_object.lock()->get_path());
      }
#endif
      THROW_UNREACHABLE("");
   }
   return res;
}

structural_objectRef port_o::find_isomorphic(const structural_objectRef key) const
{
   THROW_ASSERT(get_owner() && key->get_owner(), "Something went wrong!");
   switch(key->get_kind())
   {
      case signal_o_K:
      {
         auto* conn = GetPointer<signal_o>(key);
         for(unsigned int k = 0; k < conn->get_connected_objects_size(); k++)
            if(conn->get_port(k)->get_id() == get_id())
               return get_owner()->find_isomorphic(conn->get_port(k)->get_owner())->find_isomorphic(key);
         THROW_ERROR("Something went wrong!");
         break;
      }
      case component_o_K:
      case channel_o_K:
      {
         THROW_ASSERT(key->get_id() == get_owner()->get_id(), "Something went wrong!");
         return get_owner();
      }
      case port_o_K:
      {
         for(const auto& port : ports)
            if(port->get_id() == key->get_id())
               return port;
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_vector_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Something went wrong!");
   }
   return structural_objectRef();
}

structural_objectRef port_o::find_member(const std::string& _id, so_kind _type, const structural_objectRef _owner) const
{
   switch(_type)
   {
      case channel_o_K:
      case constant_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      case port_o_K:
      case port_vector_o_K:
      {
         for(const auto& connected_object : connected_objects)
         {
            THROW_ASSERT(port_type == port_o_K || port_type == port_vector_o_K, "inconsistently organized port");
            if(connected_object.lock()->get_kind() == _type && connected_object.lock()->get_id() == _id && connected_object.lock()->get_owner() == _owner)
               return connected_object.lock();
         }
         for(const auto& port : ports)
            if(port->get_id() == _id && port->get_owner() == _owner)
               return port;
         break;
      }
      case action_o_K:
      case component_o_K:
      case bus_connection_o_K:
      case data_o_K:
      case event_o_K:
      default:
         THROW_ERROR("Structural object not foreseen");
   }
   return structural_objectRef();
}

void port_o::copy(structural_objectRef dest) const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Copying port: " << get_path());
   structural_object::copy(dest);
   GetPointer<port_o>(dest)->dir = dir;
   GetPointer<port_o>(dest)->end = end;
   GetPointer<port_o>(dest)->pi = pi;
   GetPointer<port_o>(dest)->aligment = aligment;
   GetPointer<port_o>(dest)->bus_bundle = bus_bundle;
   GetPointer<port_o>(dest)->size_parameter = size_parameter;
   GetPointer<port_o>(dest)->is_var_args = is_var_args;
   GetPointer<port_o>(dest)->is_clock = is_clock;
   GetPointer<port_o>(dest)->is_extern = is_extern;
   GetPointer<port_o>(dest)->is_global = is_global;
   GetPointer<port_o>(dest)->is_memory = is_memory;
   GetPointer<port_o>(dest)->is_slave = is_slave;
   GetPointer<port_o>(dest)->is_master = is_master;
   GetPointer<port_o>(dest)->is_data_bus = is_data_bus;
   GetPointer<port_o>(dest)->is_addr_bus = is_addr_bus;
   GetPointer<port_o>(dest)->is_size_bus = is_size_bus;
   GetPointer<port_o>(dest)->is_tag_bus = is_tag_bus;
   GetPointer<port_o>(dest)->is_doubled = is_doubled;
   GetPointer<port_o>(dest)->is_halved = is_halved;
   GetPointer<port_o>(dest)->is_critical = is_critical;
   GetPointer<port_o>(dest)->is_reverse = is_reverse;
   if(GetPointer<port_o>(dest)->ports.size() == ports.size())
   {
      unsigned index = 0;
      for(const auto& i : ports)
      {
         structural_objectRef port = GetPointer<port_o>(dest)->get_port(index);
         i->copy(port);
         ++index;
      }
   }
   else if(GetPointer<port_o>(dest)->ports.size() == 0)
   {
      for(const auto& i : ports)
      {
         structural_objectRef port(new port_o(debug_level, dest, dir, port_o_K));
         i->copy(port);
         GetPointer<port_o>(dest)->ports.push_back(port);
      }
   }
   else if(ports.size() != 0)
      THROW_ERROR("unexpected copy: src=" + get_path() + " dest=" + dest->get_path() + " sizeSRC=" + STR(ports.size()) + " sizeDST=" + STR(GetPointer<port_o>(dest)->ports.size()));
   GetPointer<port_o>(dest)->lsb = lsb;
   /// connected_objects has to be updated outside!!!
}

void port_o::set_critical()
{
   is_critical = true;
}

bool port_o::get_critical() const
{
   return is_critical;
}

void port_o::set_reverse()
{
   is_reverse = true;
}

bool port_o::get_reverse() const
{
   return is_reverse;
}

void port_o::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   structural_object::xload(Enode, _owner, CM);
   if(CE_XVM(dir, Enode))
   {
      std::string dir_string;
      LOAD_XVFM(dir_string, Enode, dir);
      dir = to_port_direction(dir_string);
   }
   if(CE_XVM(pi, Enode))
   {
      std::string pi_string;
      LOAD_XVFM(pi_string, Enode, pi);
      pi = to_port_interface(pi_string);
   }
   if(CE_XVM(aligment, Enode))
      LOAD_XVM(aligment, Enode);
   if(CE_XVM(is_var_args, Enode))
      LOAD_XVM(is_var_args, Enode);
   if(CE_XVM(is_clock, Enode))
      LOAD_XVM(is_clock, Enode);
   if(CE_XVM(is_extern, Enode))
      LOAD_XVM(is_extern, Enode);
   if(CE_XVM(is_global, Enode))
      LOAD_XVM(is_global, Enode);
   if(CE_XVM(is_memory, Enode))
      LOAD_XVM(is_memory, Enode);
   if(CE_XVM(is_slave, Enode))
      LOAD_XVM(is_slave, Enode);
   if(CE_XVM(is_master, Enode))
      LOAD_XVM(is_master, Enode);
   if(CE_XVM(is_data_bus, Enode))
      LOAD_XVM(is_data_bus, Enode);
   if(CE_XVM(is_addr_bus, Enode))
      LOAD_XVM(is_addr_bus, Enode);
   if(CE_XVM(is_size_bus, Enode))
      LOAD_XVM(is_size_bus, Enode);
   if(CE_XVM(is_tag_bus, Enode))
      LOAD_XVM(is_tag_bus, Enode);
   if(CE_XVM(is_doubled, Enode))
      LOAD_XVM(is_doubled, Enode);
   if(CE_XVM(is_halved, Enode))
      LOAD_XVM(is_halved, Enode);
   if(CE_XVM(is_critical, Enode))
      LOAD_XVM(is_critical, Enode);
   if(CE_XVM(is_reverse, Enode))
      LOAD_XVM(is_reverse, Enode);
   if(CE_XVM(size_parameter, Enode))
      LOAD_XVM(size_parameter, Enode);

   structural_objectRef obj;
   unsigned int minBit = UINT_MAX;
   // Recourse through child nodes:
   const xml_node::node_list list = Enode->get_children();
   for(const auto& iter : list)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == GET_CLASS_NAME(port_o))
      {
         THROW_ASSERT(CE_XVM(dir, EnodeC), "Port has to have a direction." + boost::lexical_cast<std::string>(EnodeC->get_line()));
         std::string dir_string;
         LOAD_XVFM(dir_string, EnodeC, dir);
         THROW_ASSERT(dir == port_o::to_port_direction(dir_string), "port and port_vector objects has to have the same direction");
         obj = structural_objectRef(new port_o(CM->get_debug_level(), _owner, dir, port_o_K));
         obj->xload(EnodeC, obj, CM);
         ports.push_back(obj);
         auto _id = boost::lexical_cast<unsigned int>(obj->get_id());
         minBit = std::min(minBit, _id);
         port_type = port_vector_o_K;
      }
   }
   if(minBit == UINT_MAX)
      lsb = 0;
   else
      lsb = minBit;
   /// the field connected_objects has to be updated outside!!!
}

port_o::port_direction port_o::to_port_direction(const std::string& val)
{
   unsigned int i;
   for(i = 0; i < UNKNOWN; i++)
      if(val == port_directionNames[i])
         break;
   return port_direction(i);
}

port_o::port_interface port_o::to_port_interface(const std::string& val)
{
   unsigned int i;
   for(i = 0; i < UNKNOWN; i++)
      if(val == port_interfaceNames[i])
         break;
   return port_interface(i);
}

void port_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   structural_object::xwrite(Enode);
#if !RELEASE
   std::string tlm_directionality;
   std::string id_type = structural_object::get_typeRef()->id_type;
   if(id_type.find("put_if", 0) != std::string::npos)
      tlm_directionality = "->";
   else if(id_type.find("get_if", 0) != std::string::npos)
      tlm_directionality = "<-";
   else if(id_type.find("transport_if", 0) != std::string::npos)
      tlm_directionality = "<->";
   else
      tlm_directionality = "--";
   if(tlm_directionality != "--")
      WRITE_XVM(tlm_directionality, Enode);
#endif
   //   WRITE_XVM(structural_object::get_typeRef()->id_type,Enode);
   WRITE_XNVM(dir, port_directionNames[dir], Enode);
   if(pi != port_interface::PI_DEFAULT)
      WRITE_XNVM(pi, port_interfaceNames[pi], Enode);
   if(aligment != port_interface_alignment_DEFAULT)
      WRITE_XVM(aligment, Enode);
   xml_element* Enode_CO = Enode->add_child_element("connected_objects");
   for(unsigned int i = 0; i < connected_objects.size(); i++)
      WRITE_XNVM2("CON" + boost::lexical_cast<std::string>(i), connected_objects[i].lock()->get_path(), Enode_CO);
   if(is_clock != is_clock_DEFAULT)
      WRITE_XVM(is_clock, Enode);
   if(is_extern != is_extern_DEFAULT)
      WRITE_XVM(is_extern, Enode);
   if(is_global != is_global_DEFAULT)
      WRITE_XVM(is_global, Enode);
   if(is_memory != is_memory_DEFAULT)
      WRITE_XVM(is_memory, Enode);
   if(is_slave != is_slave_DEFAULT)
      WRITE_XVM(is_slave, Enode);
   if(is_master != is_master_DEFAULT)
      WRITE_XVM(is_master, Enode);
   if(is_data_bus != is_data_bus_DEFAULT)
      WRITE_XVM(is_data_bus, Enode);
   if(is_addr_bus != is_addr_bus_DEFAULT)
      WRITE_XVM(is_addr_bus, Enode);
   if(is_size_bus != is_size_bus_DEFAULT)
      WRITE_XVM(is_size_bus, Enode);
   if(is_tag_bus != is_tag_bus_DEFAULT)
      WRITE_XVM(is_tag_bus, Enode);
   if(is_doubled != is_doubled_DEFAULT)
      WRITE_XVM(is_doubled, Enode);
   if(is_halved != is_halved_DEFAULT)
      WRITE_XVM(is_halved, Enode);
   if(is_critical != is_critical_DEFAULT)
      WRITE_XVM(is_critical, Enode);
   if(is_reverse != is_reverse_DEFAULT)
      WRITE_XVM(is_reverse, Enode);
   if(is_var_args != is_var_args_DEFAULT)
      WRITE_XVM(is_var_args, Enode);
   for(auto& port : ports)
      port->xwrite(Enode);
}

#if HAVE_TECHNOLOGY_BUILT
void port_o::xwrite_attributes(xml_element* rootnode, const technology_nodeRef&
#if HAVE_EXPERIMENTAL
                                                          tn
#endif
)
{
   xml_element* pin_node = rootnode->add_child_element("pin");

   xml_element* name_node = pin_node->add_child_element("name");
   name_node->add_child_text(get_id());

   for(const auto& o : attribute_list)
   {
      const attributeRef attr = attributes[o];
      attr->xwrite(pin_node, o);
   }

#if HAVE_EXPERIMENTAL
   /// writing pin layout information
   if(GetPointer<functional_unit>(tn) && GetPointer<functional_unit>(tn)->layout_m)
   {
      GetPointer<functional_unit>(tn)->layout_m->xwrite(pin_node, get_id());
   }

   // For functional unit template we have to check that the underling functional unit has a layout
   if(GetPointer<functional_unit_template>(tn) && GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->layout_m)
   {
      GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->layout_m->xwrite(pin_node, get_id());
   }
#endif
}
#endif

void port_o::print(std::ostream& os) const
{
   PP(os, "PORT:\n");
   structural_object::print(os);
   PP(os, "[Dir: " + std::string(port_directionNames[dir]));
   if(pi != port_interface::PI_DEFAULT)
      PP(os, "[Interface: " + std::string(port_interfaceNames[pi]));
   if(aligment != port_interface_alignment_DEFAULT)
      PP(os, "[Interface: " + STR(aligment));
   if(connected_objects.size())
      PP(os, " [CON: ");
   for(const auto& connected_object : connected_objects)
      os << connected_object.lock()->get_path() + "-" + convert_so_short(connected_object.lock()->get_kind()) << " ";
   if(connected_objects.size())
      PP(os, "]");
   PP(os, "]\n");
   if(ports.size())
      PP(os, "[Ports:\n");
   for(const auto& port : ports)
      port->print(os);
   if(ports.size())
      PP(os, "]");
}

event_o::event_o(int _debug_level, const structural_objectRef o) : structural_object(_debug_level, o)
{
}

structural_objectRef event_o::find_member(const std::string&, so_kind, const structural_objectRef) const
{
   THROW_ERROR("Events do not have associated any structural object");
   return structural_objectRef();
}

void event_o::copy(structural_objectRef dest) const
{
   structural_object::copy(dest);
}

structural_objectRef event_o::find_isomorphic(const structural_objectRef) const
{
   THROW_ERROR("Something went wrong!");
   return structural_objectRef();
}

void event_o::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   structural_object::xload(Enode, _owner, CM);
}

void event_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   structural_object::xwrite(Enode);
}

void event_o::print(std::ostream& os) const
{
   PP(os, "EVENT:\n");
   structural_object::print(os);
   PP(os, "\n");
}

data_o::data_o(int _debug_level, const structural_objectRef o) : structural_object(_debug_level, o)
{
}

structural_objectRef data_o::find_member(const std::string&, so_kind, const structural_objectRef) const
{
   THROW_ERROR("data objects do not have associated any structural object");
   return structural_objectRef();
}

void data_o::copy(structural_objectRef dest) const
{
   structural_object::copy(dest);
}

structural_objectRef data_o::find_isomorphic(const structural_objectRef) const
{
   THROW_ERROR("Something went wrong!");
   return structural_objectRef();
}

void data_o::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   structural_object::xload(Enode, _owner, CM);
}

void data_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   structural_object::xwrite(Enode);
}

void data_o::print(std::ostream& os) const
{
   PP(os, "DATA:\n");
   structural_object::print(os);
   PP(os, "\n");
}

action_o::action_o(int _debug_level, const structural_objectRef o) : structural_object(_debug_level, o), function_id(0), action_type(UNKNOWN)
{
}

void action_o::add_parameter(structural_objectRef d)
{
   THROW_ASSERT(d, "NULL object received");
   parameters.push_back(d);
}

const structural_objectRef action_o::get_parameter(unsigned int n) const
{
   THROW_ASSERT(n < parameters.size(), "index out of range");
   return parameters[n];
}

unsigned int action_o::get_parameters_size() const
{
   return static_cast<unsigned int>(parameters.size());
}

void action_o::set_fun_id(unsigned int _id)
{
   function_id = _id;
}

unsigned int action_o::get_fun_id() const
{
   return function_id;
}

void action_o::set_action_type(process_type at)
{
   action_type = at;
}

action_o::process_type action_o::get_action_type() const
{
   return action_type;
}

void action_o::add_event_to_sensitivity(structural_objectRef e)
{
   THROW_ASSERT(e, "NULL object received");
   action_sensitivity.push_back(e);
}

unsigned int action_o::get_sensitivity_size() const
{
   return static_cast<unsigned int>(action_sensitivity.size());
}

const structural_objectRef action_o::get_sensitivity(unsigned int n) const
{
   THROW_ASSERT(n < action_sensitivity.size(), "index out of range");
   return action_sensitivity[n];
}

void action_o::set_scope(const std::string& sc)
{
   scope = sc;
}

const std::string& action_o::get_scope() const
{
   return scope;
}

bool action_o::get_process_nservice() const
{
   return action_type != SERVICE;
}

void action_o::copy(structural_objectRef dest) const
{
   structural_object::copy(dest);
   structural_objectRef obj;
   for(const auto& parameter : parameters)
   {
      obj = structural_objectRef(new data_o(debug_level, dest));
      parameter->copy(obj);
      GetPointer<action_o>(dest)->add_parameter(obj);
   }
   GetPointer<action_o>(dest)->function_id = function_id;
   GetPointer<action_o>(dest)->action_type = action_type;
   for(const auto& i : action_sensitivity)
   {
      obj = structural_objectRef(new event_o(debug_level, dest));
      i->copy(obj);
      GetPointer<action_o>(dest)->add_event_to_sensitivity(obj);
   }
   GetPointer<action_o>(dest)->scope = scope;
}

structural_objectRef action_o::find_isomorphic(const structural_objectRef) const
{
   THROW_ERROR("Something went wrong!");
   return structural_objectRef();
}

structural_objectRef action_o::find_member(const std::string& _id, so_kind _type, const structural_objectRef) const
{
   switch(_type)
   {
      case data_o_K:
      {
         for(const auto& parameter : parameters)
            if(parameter->get_id() == _id)
               return parameter;
         break;
      }
      case event_o_K:
      {
         for(const auto& i : action_sensitivity)
            if(i->get_id() == _id)
               return i;
         break;
      }
      case action_o_K:
      case component_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case constant_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Structural object not foreseen");
   }
   return structural_objectRef();
}

const char* action_o::process_typeNames[] = {"THREAD", "CTHREAD", "METHOD", "SERVICE", "UNKNOWN"};

void action_o::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   structural_object::xload(Enode, _owner, CM);
   LOAD_XVM(scope, Enode);
   if(CE_XVM(action_type, Enode))
   {
      unsigned int i;
      std::string val;
      LOAD_XVFM(val, Enode, action_type);
      for(i = 0; i < UNKNOWN; i++)
         if(val == process_typeNames[i])
            break;
      action_type = process_type(i);
   }
   /// someone has to take care of GM
   structural_objectRef obj;
   // Recourse through child nodes:
   const xml_node::node_list list = Enode->get_children();
   for(const auto& iter : list)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == GET_CLASS_NAME(data_o))
      {
         obj = structural_objectRef(new data_o(CM->get_debug_level(), _owner));
         obj->xload(EnodeC, obj, CM);
         GetPointer<action_o>(_owner)->add_parameter(obj);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(event_o))
      {
         obj = structural_objectRef(new event_o(CM->get_debug_level(), _owner));
         obj->xload(EnodeC, obj, CM);
         GetPointer<action_o>(_owner)->add_event_to_sensitivity(obj);
      }
   }
}

void action_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   structural_object::xwrite(Enode);
   if(parameters.size())
   {
      for(auto& parameter : parameters)
         parameter->xwrite(Enode);
   }
   /// someone has to take care of GM
   WRITE_XNVM(action_type, process_typeNames[action_type], Enode);
   if(action_sensitivity.size())
   {
      for(auto& i : action_sensitivity)
         i->xwrite(Enode);
   }
   WRITE_XVM(scope, Enode);
}

void action_o::print(std::ostream& os) const
{
   PP(os, "ACTION:\n");
   structural_object::print(os);
   PP(os, "[\n");
   if(parameters.size())
      PP(os, "Method/procedure Parameters:\n");
   for(const auto& parameter : parameters)
      parameter->print(os);
   // if (GM) GM->print(os); //too verbose
   PP(os, "Action type: " + std::string(process_typeNames[action_type]) + "\n");

   if(action_sensitivity.size())
      PP(os, "Sensitivity List:\n");
   for(const auto& i : action_sensitivity)
      i->print(os);
   os << "Scope " << scope << " ";
   os << (action_type != SERVICE ? "PROCESS" : "SERVICE");
   PP(os, "]\n");
}

constant_o::constant_o(int _debug_level, const structural_objectRef o) : structural_object(_debug_level, o)
{
}

constant_o::constant_o(int _debug_level, const structural_objectRef o, std::string _value) : structural_object(_debug_level, o), value(std::move(_value))
{
}

unsigned int constant_o::get_connected_objects_size() const
{
   return static_cast<unsigned int>(connected_objects.size());
}

structural_objectRef constant_o::get_connection(unsigned int idx) const
{
   THROW_ASSERT(idx < connected_objects.size(), "index out of range");
   return connected_objects[idx].lock();
}

void constant_o::add_connection(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT(p->get_kind() == port_o_K || p->get_kind() == signal_o_K, "constant can be connected only to ports and signals, but not to " + std::string(p->get_kind_text()));
   /// check if the object is already into the list
   for(auto& connected_object : connected_objects)
      if(connected_object.lock() == p)
         return;
   connected_objects.push_back(p);
}

void constant_o::copy(structural_objectRef dest) const
{
   THROW_ASSERT(dest, "NULL object received");
   structural_object::copy(dest);
   GetPointer<constant_o>(dest)->value = value;
   /// the field connected_objects has to be updated outside!!!
}

unsigned int constant_o::get_size() const
{
   return GET_TYPE_SIZE(this);
}

std::string constant_o::get_value() const
{
   return value;
}

structural_objectRef constant_o::find_member(const std::string& _id, so_kind _type, const structural_objectRef _owner) const
{
   switch(_type)
   {
      case signal_o_K:
      case port_o_K:
      {
         for(const auto& connected_object : connected_objects)
            if(connected_object.lock()->get_kind() == _type && connected_object.lock()->get_id() == _id && connected_object.lock()->get_owner() == _owner)
               return connected_object.lock();
         break;
      }
      case action_o_K:
      case component_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_vector_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Structural object not foreseen");
   }
   return structural_objectRef();
}

structural_objectRef constant_o::find_isomorphic(const structural_objectRef key) const
{
   THROW_ASSERT(get_owner() && key->get_owner(), "Something went wrong!");
   switch(key->get_kind())
   {
      case signal_o_K:
      {
         auto* conn = GetPointer<signal_o>(key);
         for(unsigned int k = 0; k < conn->get_connected_objects_size(); k++)
            if(conn->get_port(k)->get_id() == get_id())
               return get_owner()->find_isomorphic(conn->get_port(k)->get_owner())->find_isomorphic(key);
         THROW_ERROR("Something went wrong!");
         break;
      }
      case component_o_K:
      case channel_o_K:
      {
         THROW_ASSERT(key->get_id() == get_owner()->get_id(), "Something went wrong!");
         return get_owner();
      }
      case action_o_K:
      case bus_connection_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Something went wrong!");
   }
   return structural_objectRef();
}

void constant_o::xload(const xml_element* Enode, structural_objectRef, structural_managerRef const&)
{
   if(CE_XVM(value, Enode))
      LOAD_XVM(value, Enode);
   std::string id_string;
   if(CE_XVM(id, Enode))
      LOAD_XVFM(id_string, Enode, id);
   else
      id_string = value;
   set_id(id_string);
   /// the field connected_objects has to be updated outside!!!
}

void constant_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   structural_object::xwrite(Enode);
   WRITE_XVM(value, Enode);
   xml_element* Enode_CO = Enode->add_child_element("connected_objects");
   for(unsigned int i = 0; i < connected_objects.size(); i++)
      WRITE_XNVM2("CON" + boost::lexical_cast<std::string>(i), connected_objects[i].lock()->get_path(), Enode_CO);
}

void constant_o::print(std::ostream& os) const
{
   PP(os, "CONSTANT:\n");
   structural_object::print(os);
   PP(os, "[\n");
   PP(os, "Value: " + value + "; ");
   if(connected_objects.size())
      PP(os, " [CON: ");
   for(const auto& connected_object : connected_objects)
      os << connected_object.lock()->get_path() + "-" + convert_so_short(connected_object.lock()->get_kind()) << " ";
   if(connected_objects.size())
      PP(os, "]");
   PP(os, "]\n");
}

signal_o::signal_o(int _debug_level, const structural_objectRef o, so_kind _signal_type) : structural_object(_debug_level, o), is_critical(false), lsb(0), signal_type(_signal_type)
{
}

void signal_o::set_critical()
{
   is_critical = true;
}

bool signal_o::get_critical() const
{
   return is_critical;
}

void signal_o::add_port(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT(GetPointer<port_o>(p), "A port is expected");
   for(auto& connected_object : connected_objects)
   {
      if(connected_object.lock() == p)
         return;
   }
   connected_objects.push_back(p);
}

const structural_objectRef signal_o::get_port(unsigned int n) const
{
   THROW_ASSERT(n < connected_objects.size(), "index out of range");
   return connected_objects[n].lock();
}

structural_objectRef signal_o::get_port(unsigned int n)
{
   THROW_ASSERT(n < connected_objects.size(), "index out of range");
   return connected_objects[n].lock();
}

void signal_o::remove_port(structural_objectRef s)
{
   THROW_ASSERT(s, "NULL object received");
   THROW_ASSERT(GetPointer<port_o>(s), "A port is expected");
   auto del = connected_objects.begin();
   for(; del != connected_objects.end(); ++del)
   {
      if((*del).lock() == s)
         break;
   }
   if(del != connected_objects.end())
   {
      connected_objects.erase(del);
   }
}

bool signal_o::is_connected(structural_objectRef s) const
{
   THROW_ASSERT(s, "NULL object received");
   if(connected_objects.size() == 0)
      return false;
   auto del = connected_objects.begin();
   for(; del != connected_objects.end(); ++del)
   {
      if((*del).lock() == s)
         return true;
   }
   return false;
}

void signal_o::substitute_port(structural_objectRef old_conn, structural_objectRef new_conn)
{
   auto del = connected_objects.begin();
   for(; del != connected_objects.end(); ++del)
   {
      if((*del).lock() == new_conn)
         break;
   }
   if(del != connected_objects.end())
   {
      auto del_old = connected_objects.begin();
      for(; del_old != connected_objects.end(); ++del_old)
      {
         if((*del_old).lock() == old_conn)
            break;
      }
      if(del_old != connected_objects.end())
         connected_objects.erase(del_old);
   }
   else
   {
      for(unsigned int i = 0; i < connected_objects.size(); i++)
      {
         if(connected_objects[i].lock() == old_conn)
            connected_objects[i] = new_conn;
         else
         {
            THROW_ASSERT(GetPointer<port_o>(connected_objects[i].lock()), "Expected port");
         }
      }
   }
}

unsigned int signal_o::get_connected_objects_size() const
{
   return static_cast<unsigned int>(connected_objects.size());
}

bool signal_o::is_full_connected() const
{
   if(connected_objects.size() <= 1)
      return false;
   bool in_port = false;
   bool out_port = false;
   auto it_end = connected_objects.end();
   auto it2_end = connected_objects.end();
   for(auto it = connected_objects.begin(); it != it_end; ++it)
   {
      if(GetPointer<port_o>((*it).lock()))
      {
         auto* port = GetPointer<port_o>((*it).lock());
         if(port->get_port_direction() == port_o::IN || port->get_port_direction() == port_o::IO)
         {
            in_port = true;
            break;
         }
      }
   }
   for(auto it = connected_objects.begin(); it != it_end; ++it)
   {
      if(GetPointer<port_o>((*it).lock()))
      {
         auto* port = GetPointer<port_o>((*it).lock());
         if(port->get_port_direction() == port_o::OUT || port->get_port_direction() == port_o::IO)
         {
            out_port = true;
            break;
         }
      }
   }
   if(in_port && out_port)
      return true;
   // At this point ports are all input or all output
   for(auto it = connected_objects.begin(); it != it_end; ++it)
   {
      if(GetPointer<port_o>((*it).lock()))
      {
         auto* first_port = GetPointer<port_o>((*it).lock());
         structural_objectRef first_owner = first_port->get_owner();
         for(auto it2 = connected_objects.begin(); it2 != it2_end; ++it2)
         {
            if(GetPointer<port_o>((*it2).lock()))
            {
               auto* second_port = GetPointer<port_o>((*it2).lock());
               structural_objectRef second_owner = second_port->get_owner();
               if(first_owner == second_owner->get_owner() || second_owner == first_owner->get_owner())
                  return true;
            }
         }
      }
   }
   return false;
}

structural_objectRef signal_o::find_member(const std::string& _id, so_kind _type, const structural_objectRef _owner) const
{
   switch(_type)
   {
      case port_vector_o_K:
      case port_o_K:
      {
         for(const auto& connected_object : connected_objects)
            if(connected_object.lock()->get_id() == _id && connected_object.lock()->get_owner() == _owner)
               return connected_object.lock();
         break;
      }
      case signal_o_K:
      {
         for(const auto& signal : signals_)
            if(signal->get_id() == _id && signal->get_owner() == _owner)
               return signal;
         break;
      }
      case action_o_K:
      case component_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Structural object not foreseen " + _owner->get_kind_text());
   }
   return structural_objectRef();
}

void signal_o::copy(structural_objectRef dest) const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Copying signal: " << get_path());
   structural_object::copy(dest);
   if(is_critical)
      GetPointer<signal_o>(dest)->set_critical();
   /// someone has to take care of connected_objects

   for(const auto& signal : signals_)
   {
      structural_objectRef sig(new signal_o(debug_level, dest, signal_o_K));
      signal->copy(sig);
      GetPointer<signal_o>(dest)->signals_.push_back(sig);
   }
   GetPointer<signal_o>(dest)->lsb = lsb;
}

structural_objectRef signal_o::find_isomorphic(const structural_objectRef key) const
{
   THROW_ASSERT(get_owner() && key->get_owner(), "Something went wrong!");
   switch(key->get_kind())
   {
      case port_o_K:
      {
         /// Please pay attention to this: connected_objects are not built in this moment!
         /// check first the if key is part of a port vector
         if(key->get_owner()->get_kind() == port_vector_o_K)
         {
            if(key->get_owner()->get_owner()->get_id() == get_owner()->get_id()) /// port_vector at the same level
            {
               /// primary ports
               return get_owner()->find_isomorphic(key->get_owner())->find_isomorphic(key);
            }
            else
            {
               return get_owner()->find_isomorphic(key->get_owner()->get_owner())->find_isomorphic(key->get_owner())->find_isomorphic(key);
            }
         }
         else if(key->get_owner()->get_id() == get_owner()->get_id()) /// simple port at the same level
            /// search key in the owner
            return get_owner()->find_isomorphic(key);
         else
            /// search the owner of the key and then the key
            return get_owner()->find_isomorphic(key->get_owner())->find_isomorphic(key);
         break;
      }
      case signal_o_K:
      {
         for(const auto& signal : signals_)
            if(signal->get_id() == key->get_id())
               return signal;
         break;
      }
      case action_o_K:
      case component_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_vector_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Something went wrong!");
   }
   return structural_objectRef();
}

void signal_o::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   structural_object::xload(Enode, _owner, CM);
   /// someone has to take care of the connected_objects
   std::string _id = get_id();
   if(_id.find("\\") != std::string::npos)
   {
      structural_objectRef module_own = get_owner();
      if(module_own->get_kind() != component_o_K)
         module_own = module_own->get_owner();

      std::string base = _id;
      boost::replace_all(base, "\\", "");
      base = base.substr(0, base.find_first_of('['));
      std::string element = _id;
      element = element.substr(element.find_first_of('[') + 1, element.size());
      element = element.substr(0, element.find_first_of(']'));

      legalize(_id);

      structural_objectRef pv = module_own->find_member(base, port_vector_o_K, module_own);
      THROW_ASSERT(pv, "port vector " + base + " not found");
      structural_objectRef p = pv->find_member(element, port_o_K, pv);
      THROW_ASSERT(p, "port element " + element + " not found");
      GetPointer<signal_o>(_owner)->add_port(p);
      GetPointer<port_o>(p)->add_connection(_owner);

      set_id(_id);
   }
   // Recourse through child nodes:
   unsigned int minBit = UINT_MAX;

   const xml_node::node_list list = Enode->get_children();
   for(const auto& iter : list)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == GET_CLASS_NAME(signal_o))
      {
         structural_objectRef obj = structural_objectRef(new signal_o(CM->get_debug_level(), _owner, signal_o_K));
         obj->xload(EnodeC, obj, CM);
         signals_.push_back(obj);
         auto sig_id = boost::lexical_cast<unsigned int>(obj->get_id());
         minBit = std::min(minBit, sig_id);
         signal_type = signal_vector_o_K;
      }
   }
   if(minBit == UINT_MAX)
      lsb = 0;
   else
      lsb = minBit;
}

void signal_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   structural_object::xwrite(Enode);
   xml_element* Enode_CO = Enode->add_child_element("connected_objects");
   for(unsigned int i = 0; i < connected_objects.size(); i++)
      WRITE_XNVM2("CON" + boost::lexical_cast<std::string>(i), connected_objects[i].lock()->get_path(), Enode_CO);
   for(auto& signal : signals_)
      signal->xwrite(Enode);
}

void signal_o::print(std::ostream& os) const
{
   PP(os, "SIGNAL:\n");
   structural_object::print(os);
   if(connected_objects.size())
      PP(os, "[CON: ");
   for(const auto& connected_object : connected_objects)
      os << connected_object.lock()->get_path() + "-" + convert_so_short(connected_object.lock()->get_kind()) << " ";
   if(connected_objects.size())
      PP(os, "]\n");

   if(signals_.size())
      PP(os, "[Signals:\n");
   for(const auto& signal : signals_)
      signal->print(os);
   if(signals_.size())
      PP(os, "]");
}

void signal_o::add_n_signals(unsigned int n_signals, structural_objectRef _owner)
{
   THROW_ASSERT(!signals_.size(), "port vector has been already specialized");
   THROW_ASSERT(_owner.get() == this, "owner and this has to be the same object");
   THROW_ASSERT(n_signals != PARAMETRIC_SIGNAL, "a number of signal different from PARAMETRIC_SIGNAL is expected");
   THROW_ASSERT(get_typeRef(), "the port vector has to have a type descriptor");
   signals_.resize(n_signals);
   structural_objectRef p;
   for(unsigned int i = 0; i < n_signals; i++)
   {
      p = structural_objectRef(new signal_o(debug_level, _owner, signal_o_K));
      p->set_type(get_typeRef());
      p->set_id(boost::lexical_cast<std::string>(i));
      signals_[i] = p;
   }
   THROW_ASSERT(signal_type == signal_vector_o_K, "inconsistent data structure");
}

const structural_objectRef signal_o::get_signal(unsigned int n) const
{
   THROW_ASSERT(signals_.size(), "Signals with zero size");
   THROW_ASSERT(n < signals_.size(), "index " + STR(n) + " out of range [0:" + STR(signals_.size() - 1) + " in signal " + get_path());
   return signals_[n];
}

const structural_objectRef signal_o::get_positional_signal(unsigned int n) const
{
   THROW_ASSERT(n - lsb < signals_.size(), "index out of range");
   return signals_[n - lsb];
}

unsigned int signal_o::get_signals_size() const
{
   THROW_ASSERT(signals_.size(), "port vector has to be specialized");
   return static_cast<unsigned int>(signals_.size());
}

module::module(int _debug_level, const structural_objectRef o) : structural_object(_debug_level, o), last_position_port(0), is_critical(false), is_generated(false), multi_unit_multiplicity(0)
{
}

unsigned int module::get_num_ports() const
{
   return last_position_port;
}

void module::set_critical()
{
   is_critical = true;
}

bool module::get_critical() const
{
   return is_critical;
}

void module::set_generated()
{
   is_generated = true;
}

bool module::get_generated() const
{
   return is_generated;
}

void module::set_multi_unit_multiplicity(unsigned int value)
{
   multi_unit_multiplicity = value;
}

unsigned int module::get_multi_unit_multiplicity() const
{
   return multi_unit_multiplicity;
}

structural_objectRef module::get_positional_port(unsigned int index) const
{
   THROW_ASSERT(positional_map.find(index) != positional_map.end(), "no port at index " + boost::lexical_cast<std::string>(index));
   return positional_map.find(index)->second;
}

void module::add_in_port(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT((GetPointer<port_o>(p) && GetPointer<port_o>(p)->get_port_direction() == port_o::IN), "The parameter p is not an input port");
   THROW_ASSERT(p->get_owner().get() == this, "owner mismatch");
   in_ports.push_back(p);
   positional_map[last_position_port] = p;
   last_position_port++;
}

const structural_objectRef module::get_in_port(unsigned int n) const
{
   THROW_ASSERT(n < in_ports.size(), get_path() + ": index out of range (" + STR(n) + "/" + STR(in_ports.size()) + ") in " + get_path() + " of type " + get_typeRef()->id_type);
   return in_ports[n];
}

unsigned int module::get_in_port_size() const
{
   return static_cast<unsigned int>(in_ports.size());
}

void module::add_out_port(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT((GetPointer<port_o>(p) && GetPointer<port_o>(p)->get_port_direction() == port_o::OUT), "The parameter p is not an output port");
   THROW_ASSERT(p->get_owner().get() == this, "owner mismatch");
   out_ports.push_back(p);
   positional_map[last_position_port] = p;
   last_position_port++;
}

const structural_objectRef module::get_out_port(unsigned int n) const
{
   THROW_ASSERT(n < out_ports.size(), "index out of range");
   return out_ports[n];
}

unsigned int module::get_out_port_size() const
{
   return static_cast<unsigned int>(out_ports.size());
}

void module::add_in_out_port(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT((GetPointer<port_o>(p) && GetPointer<port_o>(p)->get_port_direction() == port_o::IO), "The parameter p is not an input-output port");
   THROW_ASSERT(p->get_owner().get() == this, "owner mismatch");
   in_out_ports.push_back(p);
   positional_map[last_position_port] = p;
   last_position_port++;
}

const structural_objectRef module::get_in_out_port(unsigned int n) const
{
   THROW_ASSERT(n < in_out_ports.size(), "index out of range");
   return in_out_ports[n];
}

unsigned int module::get_in_out_port_size() const
{
   return static_cast<unsigned int>(in_out_ports.size());
}

void module::add_gen_port(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT((GetPointer<port_o>(p) && GetPointer<port_o>(p)->get_port_direction() == port_o::GEN), "The parameter p is not a generic port");
   THROW_ASSERT(p->get_owner().get() == this, "owner mismatch");
   gen_ports.push_back(p);
   positional_map[last_position_port] = p;
   last_position_port++;
}

const structural_objectRef module::get_gen_port(unsigned int n) const
{
   THROW_ASSERT(n < gen_ports.size(), "index out of range");
   return gen_ports[n];
}

unsigned int module::get_gen_port_size() const
{
   return static_cast<unsigned int>(gen_ports.size());
}

void module::remove_port(const std::string& _id)
{
   auto num_port = static_cast<unsigned int>(positional_map.size());
   structural_objectRef port;
   for(auto& l : positional_map)
   {
      if(l.second->get_id() == _id)
      {
         num_port = l.first;
         port = l.second;
         break;
      }
   }
   THROW_ASSERT(port, "port not found: " + get_path() + "->" + _id);

   std::map<unsigned int, structural_objectRef> _positional_map = positional_map;
   positional_map.clear();
   for(auto& l : _positional_map)
   {
      if(l.first == num_port)
         continue;
      if(l.first < num_port)
         positional_map[l.first] = l.second;
      else
         positional_map[l.first - 1] = l.second;
   }
   last_position_port--;

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::IN)
   {
      bool found = false;
      for(unsigned int i = 0; i < in_ports.size(); i++)
      {
         if(in_ports[i]->get_id() == _id || found)
         {
            found = true;
            if(i == in_ports.size() - 1)
               in_ports.pop_back();
            else
               in_ports[i] = in_ports[i + 1];
         }
      }
   }

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::OUT)
   {
      bool found = false;
      for(unsigned int i = 0; i < out_ports.size(); i++)
      {
         if(out_ports[i]->get_id() == _id || found)
         {
            found = true;
            if(i == out_ports.size() - 1)
               out_ports.pop_back();
            else
               out_ports[i] = out_ports[i + 1];
         }
      }
   }

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::IO)
   {
      bool found = false;

      for(unsigned int i = 0; i < in_out_ports.size(); i++)
      {
         if(in_out_ports[i]->get_id() == _id || found)
         {
            found = true;
            if(i == in_out_ports.size() - 1)
               in_out_ports.pop_back();
            else
               in_out_ports[i] = in_out_ports[i + 1];
         }
      }
   }

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::GEN)
   {
      bool found = false;
      for(unsigned int i = 0; i < gen_ports.size(); i++)
      {
         if(gen_ports[i]->get_id() == _id || found)
         {
            found = true;
            if(i == gen_ports.size() - 1)
               gen_ports.pop_back();
            else
               gen_ports[i] = gen_ports[i + 1];
         }
      }
   }
}

void module::add_internal_object(structural_objectRef c)
{
   THROW_ASSERT(c, "NULL object received");
   THROW_ASSERT(c->get_owner().get() == this, "owner mismatch " + c->get_path() + " vs " + get_path());
   internal_objects.push_back(c);
   switch(c->get_kind())
   {
      case constant_o_K:
      {
         index_constants[c->get_id()] = c;
         break;
      }
      case signal_vector_o_K:
      case signal_o_K:
      {
         index_signals[c->get_id()] = c;
         break;
      }
      case component_o_K:
      {
         index_components[c->get_id()] = c;
         break;
      }
      case channel_o_K:
      {
         index_channels[c->get_id()] = c;
         break;
      }
      case bus_connection_o_K:
      {
         index_bus_connections[c->get_id()] = c;
         break;
      }
      case action_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      default:
         THROW_ERROR("Unexpected component: " + std::string(c->get_kind_text()));
   }
   set_black_box(false);
}

void module::remove_internal_object(structural_objectRef s)
{
   THROW_ASSERT(s, "NULL object received");
   auto del = std::find(internal_objects.begin(), internal_objects.end(), s);
   if(del != internal_objects.end())
   {
      internal_objects.erase(del);
   }
   switch(s->get_kind())
   {
      case signal_o_K:
         index_signals.erase(s->get_id());
         break;
      case component_o_K:
         index_components.erase(s->get_id());
         break;
      case channel_o_K:
         index_channels.erase(s->get_id());
         break;
      case bus_connection_o_K:
         index_bus_connections.erase(s->get_id());
         break;
      case action_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Unexpected component");
   }
}

const structural_objectRef module::get_internal_object(unsigned int n) const
{
   THROW_ASSERT(n < internal_objects.size(), "index out of range");
   return internal_objects[n];
}

unsigned int module::get_internal_objects_size() const
{
   return static_cast<unsigned int>(internal_objects.size());
}

void module::add_process(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT(p->get_kind() == action_o_K, "list of processes can have only object of type action_o");
   THROW_ASSERT(p->get_owner().get() == this, "owner mismatch");
   list_of_process.push_back(p);
}

const structural_objectRef module::get_process(unsigned int n) const
{
   THROW_ASSERT(n < list_of_process.size(), "index out of range");
   return list_of_process[n];
}

unsigned int module::get_process_size() const
{
   return static_cast<unsigned int>(list_of_process.size());
}

void module::add_service(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT(p->get_kind() == action_o_K, "list of services can have only object of type action_o");
   THROW_ASSERT(p->get_owner().get() == this, "owner mismatch");
   list_of_service.push_back(p);
}

const structural_objectRef module::get_service(unsigned int n) const
{
   THROW_ASSERT(n < list_of_service.size(), "index out of range");
   return list_of_service[n];
}

unsigned int module::get_service_size() const
{
   return static_cast<unsigned int>(list_of_service.size());
}

void module::add_event(structural_objectRef e)
{
   THROW_ASSERT(e, "NULL object received");
   THROW_ASSERT(e->get_kind() == event_o_K, "list of events can have only object of type event_o");
   THROW_ASSERT(e->get_owner().get() == this, "owner mismatch");
   list_of_event.push_back(e);
}

const structural_objectRef module::get_event(unsigned int n) const
{
   THROW_ASSERT(n < list_of_event.size(), "index out of range");
   return list_of_event[n];
}

unsigned int module::get_event_size() const
{
   return static_cast<unsigned int>(list_of_event.size());
}

void module::add_local_data(structural_objectRef d)
{
   THROW_ASSERT(d, "NULL object received");
   THROW_ASSERT(d->get_kind() == data_o_K, "Local data can have only object of type data_o");
   THROW_ASSERT(d->get_owner().get() == this, "owner mismatch");
   local_data.push_back(d);
}

const structural_objectRef module::get_local_data(unsigned int n) const
{
   THROW_ASSERT(n < local_data.size(), "index out of range");
   return local_data[n];
}

unsigned int module::get_local_data_size() const
{
   return static_cast<unsigned int>(local_data.size());
}

void module::set_NP_functionality(NP_functionalityRef f)
{
   NP_descriptions = f;
   if(get_black_box() and (f->exist_NP_functionality(NP_functionality::FSM) or f->exist_NP_functionality(NP_functionality::FSM_CS) or f->exist_NP_functionality(NP_functionality::SC_PROVIDED) or f->exist_NP_functionality(NP_functionality::VHDL_PROVIDED) or
                           f->exist_NP_functionality(NP_functionality::VERILOG_PROVIDED) or f->exist_NP_functionality(NP_functionality::SYSTEM_VERILOG_PROVIDED) or f->exist_NP_functionality(NP_functionality::VHDL_FILE_PROVIDED) or
                           f->exist_NP_functionality(NP_functionality::VERILOG_FILE_PROVIDED) or f->exist_NP_functionality(NP_functionality::FLOPOCO_PROVIDED)))
      set_black_box(false);
}

const NP_functionalityRef& module::get_NP_functionality() const
{
   return NP_descriptions;
}

void module::get_NP_library_parameters(structural_objectRef _owner, std::vector<std::pair<std::string, structural_objectRef>>& computed_parameters) const
{
   std::vector<std::string> param;
   NP_descriptions->get_library_parameters(param);
   std::vector<std::string>::const_iterator it_end = param.end();
   for(std::vector<std::string>::const_iterator it = param.begin(); it != it_end; ++it)
   {
      structural_objectRef obj = find_member(*it, port_vector_o_K, _owner);
      computed_parameters.push_back(std::make_pair(*it, obj));
   }
}

structural_objectRef module::find_member(const std::string& _id, so_kind _type, const structural_objectRef ASSERT_PARAMETER(_owner)) const
{
   THROW_ASSERT(_owner && _owner.get() == this, "owner mismatch");
   switch(_type)
   {
      case port_o_K:
      case port_vector_o_K:
      {
         for(const auto& in_port : in_ports)
            if(in_port->get_id() == _id)
               return in_port;
         for(const auto& out_port : out_ports)
            if(out_port->get_id() == _id)
               return out_port;
         for(const auto& in_out_port : in_out_ports)
            if(in_out_port->get_id() == _id)
               return in_out_port;
         for(const auto& gen_port : gen_ports)
            if(gen_port->get_id() == _id)
               return gen_port;
         break;
      }
      case component_o_K:
      {
         auto it = index_components.find(_id);
         if(it != index_components.end())
            return it->second;
         break;
      }
      case channel_o_K:
      {
         auto it = index_channels.find(_id);
         if(it != index_channels.end())
            return it->second;
         break;
      }
      case constant_o_K:
      {
         auto it = index_constants.find(_id);
         if(it != index_constants.end())
            return it->second;
         break;
      }
      case signal_vector_o_K:
      case signal_o_K:
      {
         auto it = index_signals.find(_id);
         if(it != index_signals.end())
            return it->second;
         break;
      }
      case bus_connection_o_K:
      {
         auto it = index_bus_connections.find(_id);
         if(it != index_bus_connections.end())
            return it->second;
         break;
      }
      case data_o_K:
      {
         for(const auto& i : local_data)
            if(i->get_id() == _id)
               return i;
         break;
      }
      case event_o_K:
      {
         for(const auto& i : list_of_event)
            if(i->get_id() == _id)
               return i;
         break;
      }
      case action_o_K:
      {
         for(const auto& list_of_proces : list_of_process)
            if(list_of_proces->get_id() == _id)
               return list_of_proces;
         for(const auto& i : list_of_service)
            if(i->get_id() == _id)
               return i;
         break;
      }
      default:
         THROW_ERROR("Structural object not foreseen");
   }
   return structural_objectRef();
}

bool module::is_var_args() const
{
   unsigned int currPort = 0;
   unsigned int inPortSize = get_in_port_size();
   for(currPort = 0; currPort < inPortSize; currPort++)
      if(GetPointer<port_o>(get_in_port(currPort))->get_is_var_args())
         return true;

   return false;
}

void module::copy(structural_objectRef dest) const
{
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Copying module: " + get_path() + " (" + get_typeRef()->id_type + ")");
   structural_object::copy(dest);

   if(is_critical)
      GetPointer<module>(dest)->set_critical();
   if(is_generated)
      GetPointer<module>(dest)->set_generated();
   if(multi_unit_multiplicity)
      GetPointer<module>(dest)->set_multi_unit_multiplicity(multi_unit_multiplicity);
   structural_objectRef obj;

   /// copying of the ports of the module: be aware of respecting the initial order of the ports
#ifndef NDEBUG
   if(last_position_port)
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - copying ports: " << last_position_port);
#endif
   for(unsigned int i = 0; i < last_position_port; i++)
   {
      THROW_ASSERT(positional_map.find(i) != positional_map.end(), "port " + boost::lexical_cast<std::string>(i) + " does not exist");
      const structural_objectRef port = positional_map.find(i)->second;
      port_o::port_direction dir = port_o::GEN;
      if(port->get_kind() == port_o_K)
      {
         dir = GetPointer<port_o>(port)->get_port_direction();
         obj = structural_objectRef(new port_o(debug_level, dest, dir, port_o_K));
      }
      else if(port->get_kind() == port_vector_o_K)
      {
         dir = GetPointer<port_o>(port)->get_port_direction();
         obj = structural_objectRef(new port_o(debug_level, dest, dir, port_vector_o_K));
      }
      else
         THROW_ERROR("Not expected object type: " + std::string(port->get_kind_text()));
      port->copy(obj);

      switch(dir)
      {
         case port_o::IN:
         {
            GetPointer<module>(dest)->add_in_port(obj);
            break;
         }
         case port_o::OUT:
         {
            GetPointer<module>(dest)->add_out_port(obj);
            break;
         }
         case port_o::IO:
         {
            GetPointer<module>(dest)->add_in_out_port(obj);
            break;
         }
         case port_o::GEN:
         case port_o::TLM_IN:
         case port_o::TLM_INOUT:
         case port_o::TLM_OUT:
         case port_o::UNKNOWN:
         default:
         {
            THROW_ERROR("Not supported port direction");
         }
      }
   }

   /// copy all the internal objects
#ifndef NDEBUG
   if(internal_objects.size())
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - copying internal objects: " << internal_objects.size());
#endif
   for(auto int_obj : internal_objects)
   {
      switch(int_obj->get_kind())
      {
         case signal_o_K:
         {
            obj = structural_objectRef(new signal_o(debug_level, dest, signal_o_K));
            break;
         }
         case signal_vector_o_K:
         {
            obj = structural_objectRef(new signal_o(debug_level, dest, signal_vector_o_K));
            break;
         }
         case constant_o_K:
         {
            obj = structural_objectRef(new constant_o(debug_level, dest));
            break;
         }
         case component_o_K:
         {
            obj = structural_objectRef(new component_o(debug_level, dest));
            break;
         }
         case action_o_K:
         case bus_connection_o_K:
         case channel_o_K:
         case data_o_K:
         case event_o_K:
         case port_o_K:
         case port_vector_o_K:
         default:
         {
            THROW_ERROR("Not expected type: " + std::string(int_obj->get_kind_text()));
         }
      }
      int_obj->copy(obj);
      GetPointer<module>(dest)->add_internal_object(obj);
   }

   std::string scope = get_path();
   for(unsigned int i = 0; i < last_position_port; i++)
   {
      const structural_objectRef int_obj = positional_map.find(i)->second;
      std::vector<structural_objectRef> ports;
      if(int_obj->get_kind() == port_vector_o_K)
      {
         ports.push_back(int_obj);
         for(unsigned int p = 0; p < GetPointer<port_o>(int_obj)->get_ports_size(); p++)
            ports.push_back(GetPointer<port_o>(int_obj)->get_port(p));
      }
      else
         ports.push_back(int_obj);
      for(auto port_obj : ports)
      {
         const structural_objectRef dest_port = dest->find_isomorphic(port_obj);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - copying the connections of port: " << port_obj->get_path());
         const port_o* port = GetPointer<port_o>(port_obj);
         for(unsigned int c = 0; c < port->get_connections_size(); c++)
         {
            const structural_objectRef conn_obj = port->get_connection(c);
            std::string connected_path = conn_obj->get_path();
            if(connected_path.find(scope + "/") == std::string::npos)
               continue;
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - internal connection with: " << connected_path);
            structural_objectRef dest_obj;
            if(conn_obj->get_kind() == signal_o_K || conn_obj->get_kind() == signal_vector_o_K)
            {
               // port-to-signal connection.
               dest_obj = dest->find_isomorphic(conn_obj);
               GetPointer<signal_o>(dest_obj)->add_port(dest_port);
            }
            else if(conn_obj->get_kind() == port_o_K || conn_obj->get_kind() == port_vector_o_K)
            {
               // port-to-port connection. It is a port of a submodule
               structural_objectRef conn_owner = conn_obj->get_owner();
               if(conn_owner->get_kind() == port_vector_o_K and conn_owner->get_owner()->get_path() != get_path())
                  conn_owner = conn_owner->get_owner();
               THROW_ASSERT(conn_owner, "Not valid submodule");
               dest_obj = dest->find_isomorphic(conn_owner);
               dest_obj = dest_obj->find_isomorphic(conn_obj);
               GetPointer<port_o>(dest_obj)->add_connection(dest_port);
            }
            else if(conn_obj->get_kind() == constant_o_K)
            {
               // port-to-constant connection.
               dest_obj = dest->find_isomorphic(conn_obj);
               GetPointer<constant_o>(dest_obj)->add_connection(dest_port);
            }
            else
               THROW_ERROR("Connected object not yet supported " + std::string(conn_obj->get_kind_text()));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - adding connection between " << dest_port->get_path() << " and " << dest_obj->get_path());
            GetPointer<port_o>(dest_port)->add_connection(dest_obj);
         }
      }
   }

   for(const auto& index_constant : index_constants)
   {
      const structural_objectRef int_obj = index_constant.second;
      const structural_objectRef dest_el = dest->find_isomorphic(int_obj);
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - copying the connections of constant: " << int_obj->get_path());
      const constant_o* constant = GetPointer<constant_o>(int_obj);
      for(unsigned int i = 0; i < constant->get_connected_objects_size(); i++)
      {
         const structural_objectRef conn_obj = constant->get_connection(i);
         std::string connected_path = conn_obj->get_path();
         if(connected_path.find(scope + "/") == std::string::npos)
            continue;
         /// it is a port
         structural_objectRef dest_obj;
         structural_objectRef conn_owner = conn_obj->get_owner();
         if(conn_owner->get_kind() != component_o_K || conn_owner->get_path() != get_path())
         {
            if(conn_owner->get_kind() == port_vector_o_K and conn_owner->get_owner()->get_path() != get_path())
            {
               conn_owner = conn_owner->get_owner();
            }
            dest_obj = dest->find_isomorphic(conn_owner);
            dest_obj = dest_obj->find_isomorphic(conn_obj);
         }
         else
         {
            dest_obj = dest->find_isomorphic(conn_obj);
         }
         if(GetPointer<port_o>(dest_obj))
            GetPointer<port_o>(dest_obj)->add_connection(dest_el);
         else
            THROW_ERROR("Not expected object");

         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - adding connection between " << dest_el->get_path() << " and " << dest_obj->get_path());
         GetPointer<constant_o>(dest_el)->add_connection(dest_obj);
      }
   }

   for(const auto& index_signal : index_signals)
   {
      const structural_objectRef int_obj = index_signal.second;
      std::vector<structural_objectRef> signal_objs;
      if(int_obj->get_kind() == signal_vector_o_K)
      {
         for(unsigned int p = 0; p < GetPointer<signal_o>(int_obj)->get_signals_size(); p++)
            signal_objs.push_back(GetPointer<signal_o>(int_obj)->get_signal(p));
      }
      signal_objs.push_back(int_obj);
      for(auto signal_obj : signal_objs)
      {
         const structural_objectRef signal_el = dest->find_isomorphic(signal_obj);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - copying the connections of signal: " << signal_obj->get_path());
         const signal_o* sig = GetPointer<signal_o>(signal_obj);
         for(unsigned int c = 0; c < sig->get_connected_objects_size(); c++)
         {
            const structural_objectRef conn_obj = sig->get_port(c);
            std::string connected_path = conn_obj->get_path();
            if(connected_path.find(scope + "/") == std::string::npos)
               continue;
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - internal connection with: " << connected_path);
            structural_objectRef dest_obj;
            if(conn_obj->get_kind() == port_o_K || conn_obj->get_kind() == port_vector_o_K)
            {
               structural_objectRef conn_owner = conn_obj->get_owner();
               if(conn_owner->get_kind() != component_o_K || conn_owner->get_path() != get_path())
               {
                  if(conn_owner->get_kind() == port_vector_o_K and conn_owner->get_owner()->get_path() != get_path())
                  {
                     conn_owner = conn_owner->get_owner();
                  }
                  dest_obj = dest->find_isomorphic(conn_owner);
                  dest_obj = dest_obj->find_isomorphic(conn_obj);
               }
               else
               {
                  dest_obj = dest->find_isomorphic(conn_obj);
               }
               GetPointer<port_o>(dest_obj)->add_connection(signal_el);
            }
            else
               THROW_ERROR("Connected object not yet supported " + std::string(conn_obj->get_kind_text()));
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - adding connection between " << signal_el->get_path() << " and " << dest_obj->get_path());
            GetPointer<signal_o>(signal_el)->add_port(dest_obj);
         }
      }
   }

   for(auto l = index_bus_connections.begin(); l != index_bus_connections.end(); ++l)
   {
      THROW_ERROR("Copy of bus connections is not yet supported");
   }

   for(auto l = index_channels.begin(); l != index_channels.end(); ++l)
   {
      THROW_ERROR("Copy of bus connections is not yet supported");
   }

   if(NP_descriptions)
   {
      NP_functionalityRef NP = NP_functionalityRef(new NP_functionality(NP_descriptions));
      GetPointer<module>(dest)->set_NP_functionality(NP);
   }
   GetPointer<module>(dest)->set_description(description);
   GetPointer<module>(dest)->set_copyright(copyright);
   GetPointer<module>(dest)->set_authors(authors);
   GetPointer<module>(dest)->set_license(license);
}

structural_objectRef module::find_isomorphic(const structural_objectRef key) const
{
   switch(key->get_kind())
   {
      case port_vector_o_K:
      case port_o_K:
      {
         port_o::port_direction port_dir = GetPointer<port_o>(key)->get_port_direction();
         switch(port_dir)
         {
            case port_o::IN:
            {
               for(const auto& in_port : in_ports)
               {
                  if(in_port->get_id() == key->get_id())
                     return in_port;
               }
               for(const auto& in_port : in_ports)
               {
                  if(key->get_owner()->get_kind() != port_vector_o_K || in_port->get_kind() != port_vector_o_K)
                     continue;
                  if(key->get_owner()->get_id() != in_port->get_id())
                     continue;
                  structural_objectRef iso = in_port->find_isomorphic(key);
                  if(iso)
                     return iso;
               }
               THROW_ERROR("Something went wrong with module " + key->get_path() + " in " + get_path());
               break;
            }
            case port_o::OUT:
            {
               for(const auto& out_port : out_ports)
               {
                  if(out_port->get_id() == key->get_id())
                     return out_port;
               }
               for(const auto& out_port : out_ports)
               {
                  if(key->get_owner()->get_kind() != port_vector_o_K || out_port->get_kind() != port_vector_o_K)
                     continue;
                  if(key->get_owner()->get_id() != out_port->get_id())
                     continue;
                  structural_objectRef iso = out_port->find_isomorphic(key);
                  if(iso)
                     return iso;
               }
               THROW_ERROR("Something went wrong with module " + key->get_path() + " in " + get_path());
               break;
            }
            case port_o::IO:
            {
               for(const auto& in_out_port : in_out_ports)
               {
                  if(in_out_port->get_id() == key->get_id())
                     return in_out_port;
               }
               for(const auto& in_out_port : in_out_ports)
               {
                  if(key->get_owner()->get_kind() != port_vector_o_K || in_out_port->get_kind() != port_vector_o_K)
                     continue;
                  if(key->get_owner()->get_id() != in_out_port->get_id())
                     continue;
                  structural_objectRef iso = in_out_port->find_isomorphic(key);
                  if(iso)
                     return iso;
               }
               THROW_ERROR("Something went wrong with module " + key->get_path() + " in " + get_path());
               break;
            }
            case port_o::GEN:
            {
               for(const auto& gen_port : gen_ports)
                  if(gen_port->get_id() == key->get_id())
                     return gen_port;
               THROW_ERROR("Something went wrong with module " + key->get_path());
               break;
            }
            case port_o::TLM_IN:
            case port_o::TLM_INOUT:
            case port_o::TLM_OUT:
            case port_o::UNKNOWN:
            default:
               THROW_ERROR("Something went wrong with module " + key->get_path());
         }
         break;
      }
      case component_o_K:
      {
         auto it = index_components.find(key->get_id());
         if(it != index_components.end())
            return it->second;
         THROW_ERROR("Something went wrong with module " + key->get_path());
         break;
      }
      case channel_o_K:
      {
         auto it = index_channels.find(key->get_id());
         if(it != index_channels.end())
            return it->second;
         THROW_ERROR("Something went wrong with module " + key->get_path());
         break;
      }
      case constant_o_K:
      {
         auto it = index_constants.find(key->get_id());
         if(it != index_constants.end())
            return it->second;
         THROW_ERROR("Something went wrong with module " + key->get_path());
         break;
      }
      case signal_vector_o_K:
      case signal_o_K:
      {
         auto it = index_signals.find(key->get_id());
         if(it != index_signals.end())
            return it->second;
         if(key->get_owner()->get_kind() == signal_vector_o_K)
         {
            it = index_signals.find(key->get_owner()->get_id());
            if(it != index_signals.end())
               return it->second->find_isomorphic(key);
         }
         THROW_ERROR("Something went wrong! " + key->get_path() + " in " + get_path());
         break;
      }
      case bus_connection_o_K:
      {
         auto it = index_bus_connections.find(key->get_id());
         if(it != index_bus_connections.end())
            return it->second;
         break;
      }
      case action_o_K:
      case data_o_K:
      case event_o_K:
      default:
         THROW_ERROR("Something went wrong: " + key->get_path());
   }
   return structural_objectRef();
}

std::vector<std::string> get_connections(const xml_element* node)
{
   std::vector<std::string> connections;
   const xml_node::node_list listC = node->get_children();
   for(const auto& itC : listC)
   {
      const auto* EnodeCC = GetPointer<const xml_element>(itC);
      if(!EnodeCC)
         continue;
      if(EnodeCC->get_name() == "connected_objects")
      {
         const xml_element::attribute_list Alist = EnodeCC->get_attributes();
         auto it_end = Alist.end();
         for(auto it = Alist.begin(); it != it_end; ++it)
         {
            connections.push_back((*it)->get_value());
         }
      }
   }
   return connections;
}

void module::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   structural_object::xload(Enode, _owner, CM);
   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Loading module: " << this->get_id());

   std::map<structural_objectRef, std::vector<std::string>> connections;

   structural_objectRef obj;
   // Recourse through child nodes:
   const xml_node::node_list list = Enode->get_children();
   for(const auto& iter : list)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter);
      if(!EnodeC or EnodeC->get_name() == GET_CLASS_NAME(structural_type_descriptor))
         continue;
      if(!EnodeC or EnodeC->get_name() == "parameter")
         continue;

      if(EnodeC->get_name() == GET_CLASS_NAME(port_o) || EnodeC->get_name() == GET_CLASS_NAME(port_vector_o))
      {
         port_o::port_direction dir;
         THROW_ASSERT(CE_XVM(dir, EnodeC), "Port has to have a direction." + boost::lexical_cast<std::string>(EnodeC->get_line()));
         std::string dir_string;
         LOAD_XVFM(dir_string, EnodeC, dir);
         dir = port_o::to_port_direction(dir_string);
         if(EnodeC->get_name() == GET_CLASS_NAME(port_o))
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Port");
            obj = structural_objectRef(new port_o(debug_level, _owner, dir, port_o_K));
            obj->xload(EnodeC, obj, CM);
            connections[obj] = get_connections(EnodeC);
         }
         else if(EnodeC->get_name() == GET_CLASS_NAME(port_vector_o))
         {
            PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Port Vector");
            obj = structural_objectRef(new port_o(debug_level, _owner, dir, port_vector_o_K));
            obj->xload(EnodeC, obj, CM);
            const xml_node::node_list listC = EnodeC->get_children();
            for(const auto& iterC : listC)
            {
               const auto* EnodeCC = GetPointer<const xml_element>(iterC);
               if(!EnodeCC || EnodeCC->get_name() != GET_CLASS_NAME(port_o))
                  continue;
               std::string id_string;
               LOAD_XVFM(id_string, EnodeCC, id);
               structural_objectRef port_obj = obj->find_member(id_string, port_o_K, obj);
               THROW_ASSERT(port_obj, "Port " + id_string + " not found in " + obj->get_path());
               connections[port_obj] = get_connections(EnodeCC);
            }
         }
         switch(dir)
         {
            case port_o::IN:
            {
               GetPointer<module>(_owner)->add_in_port(obj);
               break;
            }
            case port_o::OUT:
            {
               GetPointer<module>(_owner)->add_out_port(obj);
               break;
            }
            case port_o::IO:
            {
               GetPointer<module>(_owner)->add_in_out_port(obj);
               break;
            }
            case port_o::GEN:
            {
               GetPointer<module>(_owner)->add_gen_port(obj);
               break;
            }
            case port_o::TLM_IN:
            case port_o::TLM_INOUT:
            case port_o::TLM_OUT:
            case port_o::UNKNOWN:
            default:
            {
               THROW_ERROR("Port direction not foreseen");
            }
         }
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(component_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Component");
         obj = structural_objectRef(new component_o(debug_level, _owner));
         obj->xload(EnodeC, obj, CM);
         GetPointer<module>(_owner)->add_internal_object(obj);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(signal_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Signal");
         obj = structural_objectRef(new signal_o(debug_level, _owner, signal_o_K));
         obj->xload(EnodeC, obj, CM);
         GetPointer<module>(_owner)->add_internal_object(obj);
         connections[obj] = get_connections(EnodeC);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(signal_vector_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Signal vector");
         obj = structural_objectRef(new signal_o(debug_level, _owner, signal_vector_o_K));
         obj->xload(EnodeC, obj, CM);
         GetPointer<module>(_owner)->add_internal_object(obj);
         const xml_node::node_list listC = EnodeC->get_children();
         for(const auto& iterC : listC)
         {
            const auto* EnodeCC = GetPointer<const xml_element>(iterC);
            if(!EnodeCC || EnodeCC->get_name() != GET_CLASS_NAME(signal_o))
               continue;
            std::string id_string;
            LOAD_XVFM(id_string, EnodeCC, id);
            structural_objectRef sig_obj = obj->find_member(id_string, signal_o_K, obj);
            connections[sig_obj] = get_connections(EnodeCC);
         }
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(constant_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Constant");
         obj = structural_objectRef(new constant_o(debug_level, _owner));
         obj->xload(EnodeC, obj, CM);
         GetPointer<module>(_owner)->add_internal_object(obj);
         connections[obj] = get_connections(EnodeC);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(channel_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Channel");
         obj = structural_objectRef(new channel_o(debug_level, _owner));
         obj->xload(EnodeC, obj, CM);
         GetPointer<module>(_owner)->add_internal_object(obj);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(bus_connection_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Bus connection");
         obj = structural_objectRef(new bus_connection_o(debug_level, _owner));
         obj->xload(EnodeC, obj, CM);
         GetPointer<module>(_owner)->add_internal_object(obj);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(data_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Data");
         obj = structural_objectRef(new data_o(debug_level, _owner));
         obj->xload(EnodeC, obj, CM);
         GetPointer<module>(_owner)->add_local_data(obj);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(event_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Event");
         obj = structural_objectRef(new event_o(debug_level, _owner));
         obj->xload(EnodeC, obj, CM);
         GetPointer<module>(_owner)->add_event(obj);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(action_o))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Action");
         obj = structural_objectRef(new action_o(debug_level, _owner));
         obj->xload(EnodeC, obj, CM);
         if(GetPointer<action_o>(obj)->get_process_nservice())
            GetPointer<module>(_owner)->add_process(obj);
         else
            GetPointer<module>(_owner)->add_service(obj);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(NP_functionality))
      {
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - NP functionality");
         NP_descriptions = NP_functionalityRef(new NP_functionality());
         NP_descriptions->xload(EnodeC);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(description))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_WARNING("description is missing for " + EnodeC->get_name());
         description = text->get_content();
         xml_node::convert_escaped(description);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(copyright))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_WARNING("copyright is missing for " + EnodeC->get_name());
         copyright = text->get_content();
         xml_node::convert_escaped(copyright);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(authors))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_WARNING("authors are missing for " + EnodeC->get_name());
         authors = text->get_content();
         xml_node::convert_escaped(authors);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(license))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_WARNING("license is missing for " + EnodeC->get_name());
         license = text->get_content();
         xml_node::convert_escaped(license);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(specialized))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_WARNING("specialization identifier is missing for " + EnodeC->get_name());
         specialized = text->get_content();
         xml_node::convert_escaped(specialized);
      }
      else if(EnodeC->get_name() == GET_CLASS_NAME(multi_unit_multiplicity))
      {
         const xml_text_node* text = EnodeC->get_child_text();
         if(!text)
            THROW_WARNING("multi_unit_multiplicity identifier is missing for " + EnodeC->get_name());
         std::string multi_unit_multiplicitySTR = text->get_content();
         xml_node::convert_escaped(specialized);
         multi_unit_multiplicity = boost::lexical_cast<unsigned>(multi_unit_multiplicitySTR);
      }
      else
      {
         THROW_ERROR("Internal object not yet supported: " + EnodeC->get_name());
      }
   }

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Loading the interconnetions");
   const std::string& scope = get_path();
   // Second turn to connect ports, signals and constants
   for(auto& connection : connections)
   {
      obj = connection.first;
      THROW_ASSERT(obj, "Object not valid");
      const std::vector<std::string>& conns = connection.second;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   - Analyzing the connections of " << obj->get_path() << " (" << obj->get_kind_text() << "): " << conns.size());
      for(const auto& conn : conns)
      {
         /// if the scope is not included, it means that is an external connection. In this case, go on
         if(conn.find(scope + "/") == std::string::npos)
            continue;
         std::string connected_path = conn;
         connected_path = connected_path.substr(scope.size() + 1, connected_path.size());
         std::vector<std::string> elements = SplitString(connected_path, HIERARCHY_SEPARATOR);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     * Connected to " + connected_path << ": " << elements.size());
         structural_objectRef connnected_object;
         if(elements.size() == 1)
         {
            /// it can be connected to a primary scalar port or a constant or a signal
            if((connnected_object = find_member(elements[0], constant_o_K, _owner)))
            {
               GetPointer<constant_o>(connnected_object)->add_connection(obj);
            }
            else if((connnected_object = find_member(elements[0], signal_o_K, _owner)) and !(obj->get_kind() == signal_o_K and obj->get_path() == connnected_object->get_path()))
            {
               GetPointer<signal_o>(connnected_object)->add_port(obj);
            }
            else if((connnected_object = find_member(elements[0], port_o_K, _owner)) and !(obj->get_kind() == port_o_K and obj->get_path() == connnected_object->get_path()))
            {
               GetPointer<port_o>(connnected_object)->add_connection(obj);
            }
            else
               THROW_ERROR("Connected object " + connected_path + " cannot be found");
         }
         else if(elements.size() == 2)
         {
            /// it can be connected to a (scalar) port module or a primary vector port element or a vector signal element
            /// check if it is a module
            if((connnected_object = find_member(elements[0], component_o_K, _owner)))
            {
               if((connnected_object = connnected_object->find_member(elements[1], port_o_K, connnected_object)))
                  GetPointer<port_o>(connnected_object)->add_connection(obj);
               else
                  THROW_ERROR("Connected object " + connected_path + " cannot be found");
            }
            else if((connnected_object = find_member(elements[0], port_vector_o_K, _owner)))
            {
               if((connnected_object = connnected_object->find_member(elements[1], port_o_K, connnected_object)))
                  GetPointer<port_o>(connnected_object)->add_connection(obj);
               else
                  THROW_ERROR("Connected object " + connected_path + " cannot be found");
            }
            else if((connnected_object = find_member(elements[0], signal_vector_o_K, _owner)))
            {
               if((connnected_object = connnected_object->find_member(elements[1], signal_o_K, connnected_object)))
                  GetPointer<signal_o>(connnected_object)->add_port(obj);
               else
                  THROW_ERROR("Connected object " + connected_path + " cannot be found");
            }
            else
               THROW_ERROR("Connected object " + connected_path + " cannot be found");
         }
         else if(elements.size() == 3)
         {
            /// it can be connected to an element of a vector port of a module
            if((connnected_object = find_member(elements[0], component_o_K, _owner)))
            {
               if((connnected_object = connnected_object->find_member(elements[1], port_vector_o_K, connnected_object)))
               {
                  if((connnected_object = connnected_object->find_member(elements[2], port_o_K, connnected_object)))
                     GetPointer<port_o>(connnected_object)->add_connection(obj);
                  else
                     THROW_ERROR("Connected object " + connected_path + " cannot be found");
               }
               else
                  THROW_ERROR("Connected object " + connected_path + " cannot be found");
            }
            else
               THROW_ERROR("Connected object " + connected_path + " cannot be found");
         }
         else
         {
            THROW_ERROR("Not supported connected size: " + boost::lexical_cast<std::string>(elements.size()));
         }
         THROW_ASSERT(connnected_object, "Connected object not correctly identified: " + conn);
         PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "       - Identified connection with " + connnected_object->get_path());
         if(GetPointer<constant_o>(obj))
         {
            GetPointer<constant_o>(obj)->add_connection(connnected_object);
         }
         else if(GetPointer<port_o>(obj))
         {
            GetPointer<port_o>(obj)->add_connection(connnected_object);
         }
         else if(GetPointer<signal_o>(obj))
         {
            GetPointer<signal_o>(obj)->add_port(connnected_object);
         }
         else
            THROW_ERROR("Not expected type: " + std::string(obj->get_kind_text()));
      }
   }

#ifndef NDEBUG
   if(get_black_box())
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   Component " + get_id() + " (" + get_typeRef()->id_type + ") is a black box");
#endif
}

void module::xwrite(xml_element* rootnode)
{
   structural_object::xwrite(rootnode);

   xml_element* xml_description = rootnode->add_child_element("description");
   xml_description->add_child_text(description);
   xml_element* xml_copyright = rootnode->add_child_element("copyright");
   xml_copyright->add_child_text(copyright);
   xml_element* xml_authors = rootnode->add_child_element("authors");
   xml_authors->add_child_text(authors);
   xml_element* xml_license = rootnode->add_child_element("license");
   xml_license->add_child_text(license);
   if(specialized != "")
   {
      xml_element* xml_specialized = rootnode->add_child_element("specialized");
      xml_specialized->add_child_text(specialized);
   }

   if(in_ports.size())
   {
      for(auto& in_port : in_ports)
         in_port->xwrite(rootnode);
   }
   if(out_ports.size())
   {
      for(auto& out_port : out_ports)
         out_port->xwrite(rootnode);
   }
   if(in_out_ports.size())
   {
      for(auto& in_out_port : in_out_ports)
         in_out_port->xwrite(rootnode);
   }

   if(gen_ports.size())
   {
      for(auto& gen_port : gen_ports)
         gen_port->xwrite(rootnode);
   }
   if(internal_objects.size())
   {
      for(auto& internal_object : internal_objects)
         internal_object->xwrite(rootnode);
   }

   if(local_data.size())
   {
      for(auto& i : local_data)
         i->xwrite(rootnode);
   }

   if(list_of_event.size())
   {
      for(auto& i : list_of_event)
         i->xwrite(rootnode);
   }

   if(list_of_process.size())
   {
      for(auto& list_of_proces : list_of_process)
         list_of_proces->xwrite(rootnode);
   }

   if(list_of_service.size())
   {
      for(auto& i : list_of_service)
         i->xwrite(rootnode);
   }

   if(NP_descriptions)
      NP_descriptions->xwrite(rootnode);
}

#if HAVE_TECHNOLOGY_BUILT
void module::xwrite_attributes(xml_element* rootnode, const technology_nodeRef& tn)
{
   structural_object::xwrite_attributes(rootnode, tn);

#if HAVE_EXPERIMENTAL
   /// writing pin layout information
   if(GetPointer<functional_unit>(tn) && GetPointer<functional_unit>(tn)->layout_m)
   {
      GetPointer<functional_unit>(tn)->layout_m->xwrite(rootnode);
   }
   if(GetPointer<functional_unit_template>(tn) && GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->layout_m)
   {
      GetPointer<functional_unit>(GetPointer<functional_unit_template>(tn)->FU)->layout_m->xwrite(rootnode, get_id());
   }
#endif

   if(in_ports.size())
   {
      for(auto& in_port : in_ports)
         in_port->xwrite_attributes(rootnode, tn);
   }
   if(out_ports.size())
   {
      for(auto& out_port : out_ports)
         out_port->xwrite_attributes(rootnode, tn);
   }
   if(in_out_ports.size())
   {
      for(auto& in_out_port : in_out_ports)
         in_out_port->xwrite_attributes(rootnode, tn);
   }
}
#endif

void module::print(std::ostream& os) const
{
   PP(os, "MODULE:\n");
   structural_object::print(os);
   PP(os, "[\n");
   if(in_ports.size())
   {
      for(unsigned int i = 0; i < in_ports.size(); i++)
         os << "In " << i + 1 << ") " << in_ports[i];
   }
   if(out_ports.size())
   {
      for(unsigned int i = 0; i < out_ports.size(); i++)
         os << "Out " << i + 1 << ") " << out_ports[i];
   }
   if(in_out_ports.size())
   {
      for(unsigned int i = 0; i < in_out_ports.size(); i++)
         os << "IO " << i + 1 << ") " << in_out_ports[i];
   }

   if(gen_ports.size())
   {
      for(unsigned int i = 0; i < gen_ports.size(); i++)
         os << "GenP " << i + 1 << ") " << gen_ports[i];
   }

   if(internal_objects.size())
   {
      for(unsigned int i = 0; i < internal_objects.size(); i++)
         os << "Int " << i + 1 << ") " << internal_objects[i];
   }

   if(local_data.size())
   {
      for(unsigned int i = 0; i < local_data.size(); i++)
         os << "D " << i + 1 << ") " << local_data[i];
   }

   if(list_of_event.size())
   {
      for(unsigned int i = 0; i < list_of_event.size(); i++)
         os << "E " << i + 1 << ") " << list_of_event[i];
   }

   if(list_of_process.size())
   {
      for(unsigned int i = 0; i < list_of_process.size(); i++)
         os << "Proc " << i + 1 << ") " << list_of_process[i];
   }

   if(list_of_service.size())
   {
      for(unsigned int i = 0; i < list_of_service.size(); i++)
         os << "Serv " << i + 1 << ") " << list_of_service[i];
   }

   if(NP_descriptions)
      NP_descriptions->print(os);

   os << description << std::endl;
   os << copyright << std::endl;
   os << authors << std::endl;
   os << license << std::endl;
   PP(os, "]\n");
}

void module::change_port_direction(structural_objectRef port, port_o::port_direction pdir)
{
   THROW_ASSERT(GetPointer<port_o>(port), "Expected a port_o object");
   THROW_ASSERT(GetPointer<port_o>(port)->get_port_direction() != pdir, "No need to change the direction");
   std::string _id = port->get_id();
   if(GetPointer<port_o>(port)->get_port_direction() == port_o::IN)
   {
      bool found = false;
      for(unsigned int i = 0; i < in_ports.size(); i++)
      {
         if(in_ports[i]->get_id() == _id || found)
         {
            found = true;
            if(i == in_ports.size() - 1)
               in_ports.pop_back();
            else
               in_ports[i] = in_ports[i + 1];
         }
      }
   }
   else if(pdir == port_o::IN)
      in_ports.push_back(port);

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::OUT)
   {
      bool found = false;
      for(unsigned int i = 0; i < out_ports.size(); i++)
      {
         if(out_ports[i]->get_id() == _id || found)
         {
            found = true;
            if(i == out_ports.size() - 1)
               out_ports.pop_back();
            else
               out_ports[i] = out_ports[i + 1];
         }
      }
   }
   else if(pdir == port_o::OUT)
      out_ports.push_back(port);

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::IO)
   {
      bool found = false;

      for(unsigned int i = 0; i < in_out_ports.size(); i++)
      {
         if(in_out_ports[i]->get_id() == _id || found)
         {
            found = true;
            if(i == in_out_ports.size() - 1)
               in_out_ports.pop_back();
            else
               in_out_ports[i] = in_out_ports[i + 1];
         }
      }
   }
   else if(pdir == port_o::IO)
      in_out_ports.push_back(port);

   if(GetPointer<port_o>(port)->get_port_direction() == port_o::GEN)
   {
      bool found = false;
      for(unsigned int i = 0; i < gen_ports.size(); i++)
      {
         if(gen_ports[i]->get_id() == _id || found)
         {
            found = true;
            if(i == gen_ports.size() - 1)
               gen_ports.pop_back();
            else
               gen_ports[i] = gen_ports[i + 1];
         }
      }
   }
   else if(pdir == port_o::GEN)
      gen_ports.push_back(port);

   GetPointer<port_o>(port)->set_port_direction(pdir);
}

void module::AddParameter(const std::string& name, const std::string& default_value)
{
   if(name != MEMORY_PARAMETER)
   {
      if(not NP_descriptions)
      {
         NP_descriptions = NP_functionalityRef(new NP_functionality);
         NP_descriptions->add_NP_functionality(NP_functionality::LIBRARY, get_id());
      }
      NP_descriptions->add_NP_functionality(NP_functionality::LIBRARY, NP_descriptions->get_NP_functionality(NP_functionality::LIBRARY) + " " + name);
   }
   structural_object::AddParameter(name, default_value);
}

component_o::component_o(int _debug_level, const structural_objectRef o) : module(_debug_level, o)
{
}

structural_objectRef component_o::find_member(const std::string& _id, so_kind _type, const structural_objectRef _owner) const
{
   return module::find_member(_id, _type, _owner);
}

void component_o::copy(structural_objectRef dest) const
{
   module::copy(dest);
}

structural_objectRef component_o::find_isomorphic(const structural_objectRef key) const
{
   return module::find_isomorphic(key);
}

void component_o::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   module::xload(Enode, _owner, CM);
}

void component_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   module::xwrite(Enode);
}

#if HAVE_TECHNOLOGY_BUILT
void component_o::xwrite_attributes(xml_element* rootnode, const technology_nodeRef& tn)
{
   module::xwrite_attributes(rootnode, tn);
}
#endif

void component_o::print(std::ostream& os) const
{
   os << "COMPONENT-";
   module::print(os);
}

channel_o::channel_o(int _debug_level, const structural_objectRef o) : module(_debug_level, o)
{
}

void channel_o::add_interface(unsigned int t, const std::string& _interface)
{
   impl_interfaces[t] = _interface;
}

const std::string& channel_o::get_interface(unsigned int t) const
{
   return impl_interfaces.find(t)->second;
}

void channel_o::add_port(structural_objectRef p)
{
   THROW_ASSERT(p, "NULL object received");
   THROW_ASSERT(GetPointer<port_o>(p), "A port is expected");
   connected_objects.push_back(p);
}

const structural_objectRef channel_o::get_port(unsigned int n) const
{
   THROW_ASSERT(n < connected_objects.size(), "index out of range");
   return connected_objects[n].lock();
}

unsigned int channel_o::get_connected_objects_size() const
{
   return static_cast<unsigned int>(connected_objects.size());
}

structural_objectRef channel_o::find_member(const std::string& _id, so_kind _type, const structural_objectRef _owner) const
{
   structural_objectRef mod = module::find_member(_id, _type, _owner);
   if(mod)
      return mod;
   switch(_type)
   {
      case port_o_K:
      {
         for(const auto& connected_object : connected_objects)
            if(connected_object.lock()->get_id() == _id && connected_object.lock()->get_owner() == _owner)
               return connected_object.lock();
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case component_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         break;
   }
   return mod;
}

void channel_o::copy(structural_objectRef dest) const
{
   module::copy(dest);
   auto it_end = impl_interfaces.end();
   for(auto it = impl_interfaces.begin(); it != it_end; ++it)
   {
      GetPointer<channel_o>(dest)->add_interface(it->first, it->second);
   }
   /// someone has to take care of connected_objects
}

structural_objectRef channel_o::find_isomorphic(const structural_objectRef key) const
{
   /// to be completed
   structural_objectRef mod_find = module::find_isomorphic(key);
   if(mod_find)
      return mod_find;
   else
   {
      switch(key->get_kind())
      {
         case port_o_K:
         {
            /// Please pay attention to this: connected_objects are not built in this moment!

            if(key->get_owner()->get_id() == get_owner()->get_id()) /// object at the same level
               /// search key in the owner
               return get_owner()->find_isomorphic(key);
            else
               /// search the owner of the key and then the key
               return get_owner()->find_isomorphic(key->get_owner())->find_isomorphic(key);
            break;
         }
         case action_o_K:
         case component_o_K:
         case bus_connection_o_K:
         case channel_o_K:
         case constant_o_K:
         case data_o_K:
         case event_o_K:
         case port_vector_o_K:
         case signal_o_K:
         case signal_vector_o_K:
         default:
            THROW_ERROR("Something went wrong!");
      }
   }
   return structural_objectRef();
}

void channel_o::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   module::xload(Enode, _owner, CM);
   // Recourse through child nodes:
   const xml_node::node_list list = Enode->get_children();
   for(const auto& iter : list)
   {
      const auto* EnodeC = GetPointer<const xml_element>(iter);
      if(!EnodeC)
         continue;
      if(EnodeC->get_name() == "impl_interfaces")
      {
         const xml_element::attribute_list Alist = EnodeC->get_attributes();
         auto it_end = Alist.end();
         for(auto it = Alist.begin(); it != it_end; ++it)
         {
            impl_interfaces[boost::lexical_cast<unsigned int>((*it)->get_name().c_str() + 2)] = (*it)->get_value();
         }
      }
   }
}

void channel_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   module::xwrite(Enode);
   xml_element* Enode_II = Enode->add_child_element("impl_interfaces");
   std::map<unsigned int, std::string>::const_iterator it_end = impl_interfaces.end();
   for(std::map<unsigned int, std::string>::const_iterator it = impl_interfaces.begin(); it != it_end; ++it)
   {
      WRITE_XNVM2("II" + boost::lexical_cast<std::string>(it->first), it->second, Enode_II);
   }
   xml_element* Enode_CO = Enode->add_child_element("connected_objects");
   for(unsigned int i = 0; i < connected_objects.size(); i++)
      WRITE_XNVM2("CON" + boost::lexical_cast<std::string>(i), connected_objects[i].lock()->get_path(), Enode_CO);
}

#if HAVE_TECHNOLOGY_BUILT
void channel_o::xwrite_attributes(xml_element* rootnode, const technology_nodeRef& tn)
{
   module::xwrite_attributes(rootnode, tn);
}
#endif

void channel_o::print(std::ostream& os) const
{
   os << "CHANNEL-";
   module::print(os);
   PP(os, "[");
   if(impl_interfaces.size())
      PP(os, "List of the interfaces:\n");
   for(const auto& impl_interface : impl_interfaces)
   {
      os << " @(" << impl_interface.first << ") " << impl_interface.second;
      PP(os, "\n");
   }
   for(const auto& connected_object : connected_objects)
      os << connected_object.lock()->get_path() + "-" + convert_so_short(connected_object.lock()->get_kind()) << " ";
   PP(os, "\n]\n");
}

bus_connection_o::bus_connection_o(int _debug_level, const structural_objectRef o) : structural_object(_debug_level, o)
{
}

void bus_connection_o::add_connection(structural_objectRef c)
{
   THROW_ASSERT(c, "NULL object received");
   connections.push_back(c);
}

const structural_objectRef bus_connection_o::get_connection(unsigned int n) const
{
   THROW_ASSERT(n < connections.size(), "index out of range");
   return connections[n].lock();
}

unsigned int bus_connection_o::get_connections_size() const
{
   return static_cast<unsigned int>(connections.size());
}

structural_objectRef bus_connection_o::find_isomorphic(const structural_objectRef) const
{
   THROW_ERROR("Something went wrong!");
   return structural_objectRef();
}

structural_objectRef bus_connection_o::find_member(const std::string& _id, so_kind _type, const structural_objectRef _owner) const
{
   switch(_type)
   {
      case signal_o_K:
      case port_o_K:
      {
         for(const auto& connection : connections)
            if(connection.lock()->get_id() == _id && connection.lock()->get_owner() == _owner)
               return connection.lock();
         break;
      }
      case action_o_K:
      case component_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_vector_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Structural object not foreseen");
   }
   return structural_objectRef();
}

void bus_connection_o::copy(structural_objectRef dest) const
{
   structural_object::copy(dest);
   /// someone has to take care of connections
}

void bus_connection_o::xload(const xml_element* Enode, structural_objectRef _owner, structural_managerRef const& CM)
{
   structural_object::xload(Enode, _owner, CM);
}

void bus_connection_o::xwrite(xml_element* rootnode)
{
   xml_element* Enode = rootnode->add_child_element(get_kind_text());
   structural_object::xwrite(Enode);
}

void bus_connection_o::print(std::ostream& os) const
{
   PP(os, "BUS CONNECTION:\n");
   structural_object::print(os);
   PP(os, "[\n");
   if(connections.size())
      PP(os, "List of connections:\n");
   for(const auto& connection : connections)
      os << connection.lock()->get_path() + "-" + convert_so_short(connection.lock()->get_kind()) << " ";
   PP(os, "\n]\n");
}

void port_o::add_n_ports(unsigned int n_ports, structural_objectRef _owner)
{
   THROW_ASSERT(port_type == port_vector_o_K, "the port " + get_path() + " has to be of type port_vector_o_K");
   THROW_ASSERT(_owner.get() == this, "owner and this has to be the same object");
   THROW_ASSERT(n_ports != PARAMETRIC_PORT, "a number of port different from PARAMETRIC_PORT is expected");
   THROW_ASSERT(get_typeRef(), "the port vector has to have a type descriptor");

   unsigned int currentPortNumber = this->get_ports_size();
   for(unsigned int i = 0; i < n_ports; i++)
   {
      structural_objectRef p = structural_objectRef(new port_o(debug_level, _owner, dir, port_o_K));
      p->set_type(get_typeRef());
      p->set_id(boost::lexical_cast<std::string>(currentPortNumber + i));
      ports.push_back(p);
   }
   THROW_ASSERT(port_type == port_vector_o_K, "inconsistent data structure");
}

const structural_objectRef port_o::get_port(unsigned int n) const
{
   THROW_ASSERT(port_type == port_vector_o_K, "the port " + get_path() + " has to be of type port_vector_o_K");
   THROW_ASSERT(n < ports.size(), "index out of range: " + get_path() + " " + STR(n) + "/" + STR(ports.size()));
   return ports[n];
}

unsigned int port_o::get_ports_size() const
{
   THROW_ASSERT(port_type == port_vector_o_K, "the port " + get_path() + " has to be of type port_vector_o_K");
   return static_cast<unsigned int>(ports.size());
}

void port_o::set_port_size(unsigned int dim)
{
   get_typeRef()->size = dim;
}

unsigned int port_o::get_port_size() const
{
   return get_typeRef()->size;
}

void port_o::resize_busport(unsigned int bus_size_bitsize, unsigned int bus_addr_bitsize, unsigned int bus_data_bitsize, unsigned int bus_tag_bitsize, structural_objectRef port)
{
   if(GetPointer<port_o>(port)->get_is_data_bus())
      port->type_resize(bus_data_bitsize);
   else if(GetPointer<port_o>(port)->get_is_addr_bus())
      port->type_resize(bus_addr_bitsize);
   else if(GetPointer<port_o>(port)->get_is_size_bus())
      port->type_resize(bus_size_bitsize);
   else if(GetPointer<port_o>(port)->get_is_tag_bus())
      port->type_resize(bus_tag_bitsize);
   if(port->get_kind() == port_vector_o_K)
   {
      for(unsigned int pi = 0; pi < GetPointer<port_o>(port)->get_ports_size(); ++pi)
      {
         structural_objectRef port_d = GetPointer<port_o>(port)->get_port(pi);
         if(GetPointer<port_o>(port)->get_is_data_bus())
            port_d->type_resize(bus_data_bitsize);
         else if(GetPointer<port_o>(port)->get_is_addr_bus())
            port_d->type_resize(bus_addr_bitsize);
         else if(GetPointer<port_o>(port)->get_is_size_bus())
            port_d->type_resize(bus_size_bitsize);
         else if(GetPointer<port_o>(port)->get_is_tag_bus())
            port_d->type_resize(bus_tag_bitsize);
      }
   }
}

void port_o::resize_std_port(unsigned int bitsize_variable, unsigned int n_elements, int DEBUG_PARAMETER(debug_level), structural_objectRef port)
{
   if(n_elements == 0)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Specializing port " + port->get_path() + " " + STR(bitsize_variable));
      if(port->get_kind() == port_vector_o_K)
      {
         THROW_ASSERT(GetPointer<port_o>(port)->get_ports_size(), "Number of ports not specialized for " + port->get_path());
         for(unsigned int pi = 0; pi < GetPointer<port_o>(port)->get_ports_size(); ++pi)
         {
            structural_objectRef port_d = GetPointer<port_o>(port)->get_port(pi);
            port_d->type_resize(bitsize_variable);
         }
      }
      port->type_resize(bitsize_variable);
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Specializing port " + port->get_path() + " " + STR(bitsize_variable) + ":" + STR(n_elements));
      if(port->get_kind() == port_vector_o_K)
      {
         THROW_ASSERT(GetPointer<port_o>(port)->get_ports_size(), "Number of ports not specialized");
         for(unsigned int pi = 0; pi < GetPointer<port_o>(port)->get_ports_size(); ++pi)
         {
            structural_objectRef port_d = GetPointer<port_o>(port)->get_port(pi);
            port_d->type_resize(bitsize_variable, n_elements);
         }
      }
      port->type_resize(bitsize_variable, n_elements);
   }
}

void port_o::fix_port_properties(structural_objectRef port_i, structural_objectRef cir_port)
{
   if(GetPointer<port_o>(port_i)->get_port_interface() != port_o::port_interface::PI_DEFAULT)
      GetPointer<port_o>(cir_port)->set_port_interface(GetPointer<port_o>(port_i)->get_port_interface());
   if(GetPointer<port_o>(port_i)->get_port_alignment() != port_interface_alignment_DEFAULT)
      GetPointer<port_o>(cir_port)->set_port_alignment(GetPointer<port_o>(port_i)->get_port_alignment());
   if(GetPointer<port_o>(port_i)->get_is_extern())
      GetPointer<port_o>(cir_port)->set_is_extern(true);
   if(GetPointer<port_o>(port_i)->get_is_global())
      GetPointer<port_o>(cir_port)->set_is_global(true);
   if(GetPointer<port_o>(port_i)->get_is_memory())
      GetPointer<port_o>(cir_port)->set_is_memory(true);
   if(GetPointer<port_o>(port_i)->get_is_slave())
      GetPointer<port_o>(cir_port)->set_is_slave(true);
   if(GetPointer<port_o>(port_i)->get_is_master())
      GetPointer<port_o>(cir_port)->set_is_master(true);
   if(GetPointer<port_o>(port_i)->get_is_data_bus())
      GetPointer<port_o>(cir_port)->set_is_data_bus(true);
   if(GetPointer<port_o>(port_i)->get_is_addr_bus())
      GetPointer<port_o>(cir_port)->set_is_addr_bus(true);
   if(GetPointer<port_o>(port_i)->get_is_size_bus())
      GetPointer<port_o>(cir_port)->set_is_size_bus(true);
   if(GetPointer<port_o>(port_i)->get_is_tag_bus())
      GetPointer<port_o>(cir_port)->set_is_tag_bus(true);
   if(GetPointer<port_o>(port_i)->get_is_doubled())
      GetPointer<port_o>(cir_port)->set_is_doubled(true);
   if(GetPointer<port_o>(port_i)->get_is_halved())
      GetPointer<port_o>(cir_port)->set_is_halved(true);
}

#if HAVE_KOALA_BUILT
std::string structural_object::get_equation(const structural_objectRef out_obj, const technology_managerConstRef TM, CustomOrderedSet<structural_objectRef>& analyzed, const CustomOrderedSet<structural_objectRef>& input_ports,
                                            const CustomOrderedSet<structural_objectRef>& output_ports) const
{
   analyzed.insert(out_obj);

   std::string EQ = out_obj->get_id();

   if(input_ports.find(out_obj) != input_ports.end())
   {
      return EQ;
   }

   //    const structural_objectRef obj_owner = out_obj->get_owner();

   switch(out_obj->get_kind())
   {
      case port_o_K:
      {
         const structural_objectRef owner = this->get_owner();
         if(owner and GetPointer<port_o>(out_obj)->get_port_direction() == port_o::OUT)
         {
            const structural_type_descriptorRef STD = owner->get_typeRef();
            std::string Library = TM->get_library(STD->id_type);
            const technology_nodeRef& TN = TM->get_fu(STD->id_type, Library);
            THROW_ASSERT(TN, "Module " + owner->get_path() + " is not stored into library");
            functional_unit* fu = GetPointer<functional_unit>(TN);
            THROW_ASSERT(fu, "Module " + owner->get_path() + " is not a functional unit");
            const structural_objectRef strobj = fu->CM->get_circ();
            NP_functionalityRef NPF = GetPointer<module>(owner)->get_NP_functionality();
            if(!NPF)
            {
               NPF = GetPointer<module>(strobj)->get_NP_functionality();
               THROW_ASSERT(NPF, "Functionality not available for element " + owner->get_id());
            }
            std::string tmp = NPF->get_NP_functionality(NP_functionality::EQUATION);
            /*if (GetPointer<module>(strobj)->get_out_port_size() > 1)
               THROW_ERROR("Multi-out module not supported");
            */
            std::vector<std::string> tokens = SplitString(tmp, ";");
            for(unsigned int i = 0; i < tokens.size(); i++)
            {
               if(boost::algorithm::starts_with(tokens[i], out_obj->get_id()))
                  EQ = tokens[i].substr(tokens[i].find("=") + 1, tokens[i].size());
            }
            // EQ = NPF->get_NP_functionality(NP_functionality::EQUATION);
            for(unsigned int p = 0; p < GetPointer<module>(owner)->get_in_port_size(); p++)
            {
               const structural_objectRef inobj = GetPointer<module>(owner)->get_in_port(p);
               std::string In = inobj->get_equation(inobj, TM, analyzed, input_ports, output_ports);
               bool in_port = false;
               for(CustomOrderedSet<structural_objectRef>::iterator k = input_ports.begin(); k != input_ports.end() and !in_port; ++k)
                  if((*k)->get_id() == In)
                     in_port = true;
               if(!in_port)
                  In = "(" + In + ")";
               boost::replace_all(EQ, GetPointer<module>(strobj)->get_in_port(p)->get_id(), In);
            }
         }
         else
         {
            for(unsigned int p = 0; p < GetPointer<port_o>(out_obj)->get_connections_size(); p++)
            {
               const structural_objectRef obj = GetPointer<port_o>(out_obj)->get_connection(p);
               if(output_ports.find(obj) != output_ports.end())
                  continue;
               if(obj /* and analyzed.find(obj) == analyzed.end()*/)
               {
                  EQ = obj->get_equation(obj, TM, analyzed, input_ports, output_ports);
               }
               else
                  EQ = out_obj->get_id();
            }
         }
         break;
      }
      case signal_o_K:
      {
         for(unsigned int p = 0; p < GetPointer<signal_o>(out_obj)->get_connected_objects_size(); p++)
         {
            const structural_objectRef obj = GetPointer<signal_o>(out_obj)->get_port(p);
            if(obj and out_obj != obj /* and analyzed.find(obj) == analyzed.end()*/)
            {
               if(output_ports.find(obj) != output_ports.end() and analyzed.find(obj) == analyzed.end())
               {
                  return obj->get_id();
               }
               if(GetPointer<port_o>(obj)->get_port_direction() != port_o::OUT || obj->get_owner() == get_owner())
                  continue;
               EQ = obj->get_equation(obj, TM, analyzed, input_ports, output_ports);
            }
         }
         break;
      }
      default:
         THROW_ERROR("Not supported component " + std::string(out_obj->get_kind_text()));
   }

   return EQ;

#if 0
   switch (out_obj->get_kind())
   {
      case port_o_K:
      {
         /// get the module owner of the port
         const structural_objectRef owner = this->get_owner();
         ///this is the top of the hierarchy, it won't be in the library
         if (!owner)
         {
            THROW_ASSERT(this == out_obj->get_owner().get(), "Malformed structure");
            const structural_objectRef owner = out_obj->get_owner();
            for(unsigned int p = 0; p < GetPointer<port_o>(out_obj)->get_connections_size(); p++)
            {
               const structural_objectRef obj = GetPointer<port_o>(out_obj)->get_connection(p);
               if (obj and analyzed.find(obj) == analyzed.end())
               {
                  analyzed.insert(obj);
                  EQ = obj->get_equation(obj, TM, analyzed, input_ports);
               }
               else
                  EQ = out_obj->get_id();
            }
         }
         ///if it is an output port, compute the internal equation of the component
         else if (owner and GetPointer<port_o>(out_obj)->get_port_direction() == port_o::OUT)
         {
            analyzed.insert(owner);
            const structural_type_descriptorRef STD = owner->get_typeRef();
            const technology_nodeRef& TN = TM->get_fu(STD->id_type, "EDIF_LIB");
            THROW_ASSERT(TN, "Module " + owner->get_path() + " is not stored into library");
            functional_unit* fu = GetPointer<functional_unit>(TN);
            THROW_ASSERT(fu, "Module " + owner->get_path() + " is not a functional unit");
            const structural_objectRef strobj = fu->CM->get_circ();
            NP_functionalityRef NPF = GetPointer<module>(owner)->get_NP_functionality();
            if (!NPF)
            {
               NPF = GetPointer<module>(strobj)->get_NP_functionality();
               THROW_ASSERT(NPF, "Functionality not available for element " + owner->get_id());
            }
               std::string tmp = NPF->get_NP_functionality(NP_functionality::EQUATION);
               if (GetPointer<module>(strobj)->get_out_port_size() > 1)
                  THROW_ERROR("Multi-out module not supported");
               std::vector<std::string> tokens = SplitString(tmp, ";");
               for(unsigned int i = 0; i < tokens.size(); i++)
               {
                  if (tokens[i].find(GetPointer<module>(strobj)->get_out_port(0)->get_id()) == 0)
                     EQ = tokens[i].substr(tokens[i].find("=") + 1, tokens[i].size());
               }
               //EQ = NPF->get_NP_functionality(NP_functionality::EQUATION);
               for(unsigned int p = 0; p < GetPointer<module>(owner)->get_in_port_size(); p++)
               {
                  const structural_objectRef inobj = GetPointer<module>(owner)->get_in_port(p);
                  analyzed.insert(inobj);
                  std::string In = inobj->get_equation(inobj, TM, analyzed, input_ports);
                  boost::replace_all(EQ, GetPointer<module>(strobj)->get_in_port(p)->get_id(), In);
               }
            }
            else
            {
               for(unsigned int p = 0; p < GetPointer<port_o>(out_obj)->get_connections_size(); p++)
               {
                  const structural_objectRef obj = GetPointer<port_o>(out_obj)->get_connection(p);
                  if (obj and analyzed.find(obj) == analyzed.end())
                  {
                     analyzed.insert(obj);
                     EQ = obj->get_equation(obj, TM, analyzed, input_ports);
                  }
                  else
                     EQ = out_obj->get_id();
               }
            }
            break;
         }
         case signal_o_K:
         {
            for(unsigned int p = 0; p < GetPointer<signal_o>(out_obj)->get_connected_objects_size(); p++)
            {
               const structural_objectRef obj = GetPointer<signal_o>(out_obj)->get_port(p);
               if (obj  and analyzed.find(obj) == analyzed.end())
               {
                  analyzed.insert(obj);
                  EQ = obj->get_equation(obj, TM, analyzed, input_ports);
               }
            }
            break;
         }
         default:
            THROW_ERROR("Not supported component " + std::string(out_obj->get_kind_text()));
      }
#endif
}
#endif
