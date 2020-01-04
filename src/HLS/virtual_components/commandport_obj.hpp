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
 * @file commandport_obj.hpp
 * @brief Base class for all command ports into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef COMMANDPORT_OBJ_HPP
#define COMMANDPORT_OBJ_HPP

#include "refcount.hpp"
#include <boost/lexical_cast.hpp>
#include <utility>

#include "generic_obj.hpp"

#include "conn_binding.hpp"

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(commandport_obj);
//@}

/**
 * @class commandport_obj
 * This class manages command ports into datapath. It contains information about operation vertex giving command
 * if it is operation or a condition. It also contains information about port direction. All these command ports
 * are connected to controller.
 */
class commandport_obj : public generic_obj
{
 public:
   /// Available command types
   typedef enum
   {
      OPERATION = 0,   /// operation enable
      CONDITION,       /// conditional value. it represents a readcond if it goes to the controller or a condition if it goes to the datapath
      SWITCH,          /// switch value, it represents the value of the switch statement
      MULTIIF,         /// represents the multi conditions
      SELECTOR,        /// mux selector
      ALUSELECTOR,     /// ALU selector
      UNBOUNDED,       /// signal representing a communication for an unbounded object (function call)
      MULTI_UNBOUNDED, /// signal representing when a multi unbounded call ends
      WRENABLE         /// enable for register writing
   } command_type;

   typedef std::pair<unsigned int, vertex> data_operation_pair;
   /// describe a transition from a source state to the target state plus the tree_node of the data transferred and the operation vertex where the computation is performed
   typedef std::tuple<vertex, vertex, data_operation_pair> transition;

 private:
   /// TODO: substitute with a functor
   /// operation vertex associated with the command port signal (if type is condition or operation) or
   /// state vertex associated with the command port signal
   vertex signal;

   generic_objRef elem;

   /// It's command type for the port
   unsigned int mode;

   bool is_a_phi_write_enable;

   CustomOrderedSet<transition> activations;

   /// structural_object associated with the element inside the controller
   structural_objectRef controller_SM;

 public:
   /**
    * This is the constructor of the commandport_obj class. It initializes type for generic_obj superclass
    * @param signal_ is vertex associated to port
    * @param mode is command type
    */
   commandport_obj(const vertex& signal_, unsigned int _mode, const std::string& _name) : generic_obj(COMMAND_PORT, _name), signal(signal_), mode(_mode), is_a_phi_write_enable(false)
   {
      THROW_ASSERT(mode == OPERATION or mode == CONDITION or mode == SWITCH or mode == MULTIIF or mode == UNBOUNDED, "Command mode not allowed into this constructor");
   }

   commandport_obj(generic_objRef _elem, unsigned int _mode, const std::string& _name) : generic_obj(COMMAND_PORT, _name), elem(std::move(_elem)), mode(_mode), is_a_phi_write_enable(false)
   {
      THROW_ASSERT(mode == SELECTOR || mode == WRENABLE || mode == ALUSELECTOR or mode == MULTI_UNBOUNDED, "Selector port is wrong");
   }

   /**
    * Destructor.
    */
   ~commandport_obj() override = default;

   /**
    * Gets the vertex associated with port
    * @return reference to vertex
    */
   const vertex& get_vertex() const
   {
      THROW_ASSERT(mode == OPERATION or mode == CONDITION or mode == SWITCH or mode == MULTIIF or mode == UNBOUNDED, "Command mode not allowed");
      return signal;
   }

   /**
    * Gets command type
    * @return an integer associated with command type
    */
   unsigned int get_command_type() const
   {
      return mode;
   }

   const CustomOrderedSet<transition>& get_activations() const
   {
      return activations;
   }

   void add_activation(const transition& act)
   {
      activations.insert(act);
   }

   /**
    * Sets structural_object associated to this object
    * @param _SM is reference to structural_object to be associated
    */
   void set_controller_obj(const structural_objectRef& _SM)
   {
      controller_SM = _SM;
   }

   /**
    * Gets structural_object associated to this object
    * @return a reference to structural_object associated
    */
   const structural_objectRef& get_controller_obj() const
   {
      return controller_SM;
   }

   const generic_objRef& get_elem() const
   {
      THROW_ASSERT(mode == SELECTOR || mode == WRENABLE || mode == ALUSELECTOR || mode == MULTI_UNBOUNDED, "Selector port is wrong");
      return elem;
   }

   static const std::string get_mode_string(unsigned int _mode)
   {
      switch(_mode)
      {
         case OPERATION:
            return "OPERATION";
         case CONDITION:
            return "CONDITION";
         case SWITCH:
            return "SWITCH";
         case MULTIIF:
            return "MULTIIF";
         case UNBOUNDED:
            return "UNBOUNDED";
         case MULTI_UNBOUNDED:
            return "MULTI_UNBOUNDED";
         case SELECTOR:
            return "SELECTOR";
         case WRENABLE:
            return "WRENABLE";
         default:
            THROW_ERROR("Command mode not allowed for port");
      }
      return "";
   }

   void set_phi_write_enable()
   {
      is_a_phi_write_enable = true;
   }

   bool get_phi_write_enable()
   {
      return is_a_phi_write_enable;
   }
};

#endif
