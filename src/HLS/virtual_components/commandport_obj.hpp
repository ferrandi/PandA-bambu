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
 * @file commandport_obj.hpp
 * @brief Base class for all command ports into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef COMMANDPORT_OBJ_HPP
#define COMMANDPORT_OBJ_HPP

#include "FSMInfo.hpp"
#include "conn_binding.hpp"
#include "generic_obj.hpp"
#include "refcount.hpp"

#include <utility>

REF_FORWARD_DECL(commandport_obj);

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
   using command_type = enum {
      OPERATION = 0,          /// operation enable
      MULTIIF,                /// represents the multi conditions
      SELECTOR,               /// mux selector
      UNBOUNDED,              /// signal representing a communication for an unbounded object (function call)
      MULTI_UNBOUNDED,        /// signal representing when a multi unbounded call ends
      MULTI_UNBOUNDED_ENABLE, /// signal enabling the multi unbounded component
      WRENABLE                /// enable for register writing
   };

   using data_operation_pair = std::pair<unsigned int, gc_vertex_descriptor>;
   /// describe a transition from a source state to the target state plus the ir_node of the data transferred and the
   /// operation vertex where the computation is performed
   using transition = std::tuple<FSMInfo::state_descriptor, FSMInfo::state_descriptor, data_operation_pair>;

 private:
   /// TODO: substitute with a functor
   /// operation vertex associated with the command port signal (if type is condition or operation) or
   /// state vertex associated with the command port signal
   gc_vertex_descriptor signal;

   generic_objRef elem;

   /// It's command type for the port
   unsigned int mode;

   CustomOrderedSet<transition> activations;

   /// structural_object associated with the element inside the controller
   Wrefcount<structural_object> controller_SM;

 public:
   /**
    * This is the constructor of the commandport_obj class. It initializes type for generic_obj superclass
    * @param signal_ is vertex associated to port
    * @param _mode is command type
    * @param _name is the port name
    */
   commandport_obj(gc_vertex_descriptor signal_, unsigned int _mode, const std::string& _name)
       : generic_obj(COMMAND_PORT, _name), signal(signal_), mode(_mode)
   {
      THROW_ASSERT(mode == OPERATION or mode == MULTIIF or mode == UNBOUNDED,
                   "Command mode not allowed into this constructor");
   }

   commandport_obj(generic_objRef _elem, unsigned int _mode, const std::string& _name)
       : generic_obj(COMMAND_PORT, _name), signal(gc_null_vertex()), elem(_elem), mode(_mode)
   {
      THROW_ASSERT(mode == SELECTOR || mode == WRENABLE || mode == MULTI_UNBOUNDED or mode == MULTI_UNBOUNDED_ENABLE,
                   "Selector port is wrong");
   }

   /**
    * Gets the vertex associated with port
    * @return reference to vertex
    */
   gc_vertex_descriptor get_vertex() const
   {
      THROW_ASSERT(mode == OPERATION || mode == MULTIIF || mode == UNBOUNDED, "Command mode not allowed");
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
   const structural_objectRef get_controller_obj() const
   {
      return controller_SM.lock();
   }

   const generic_objRef& get_elem() const
   {
      THROW_ASSERT(mode == SELECTOR || mode == WRENABLE || mode == MULTI_UNBOUNDED or mode == MULTI_UNBOUNDED_ENABLE,
                   "Selector port is wrong");
      return elem;
   }

   static const std::string get_mode_string(unsigned int _mode)
   {
      switch(_mode)
      {
         case OPERATION:
            return "OPERATION";
         case MULTIIF:
            return "MULTIIF";
         case UNBOUNDED:
            return "UNBOUNDED";
         case MULTI_UNBOUNDED:
            return "MULTI_UNBOUNDED";
         case MULTI_UNBOUNDED_ENABLE:
            return "MULTI_UNBOUNDED_ENABLE";
         case SELECTOR:
            return "SELECTOR";
         case WRENABLE:
            return "WRENABLE";
         default:
            THROW_ERROR("Command mode not allowed for port");
      }
      return "";
   }
};

#endif
