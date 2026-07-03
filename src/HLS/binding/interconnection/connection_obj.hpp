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
 * @file connection_obj.hpp
 * @brief Base class for all resources into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef CONN_ELEMENT_HPP
#define CONN_ELEMENT_HPP

#include <utility>

#include "conn_binding.hpp"
#include "refcount.hpp"

/**
 * @class connection_obj
 * Generic class managing elements used to interconnect generic objects into datapath
 */
class connection_obj
{
 public:
   /// resource type
   using element_t = enum { DIRECT_CONN, BY_MUX };

 protected:
   /// type of the connection
   element_t type;

   /// Set of variables that cross the connection
   CustomOrderedSet<data_transfer> live_variable;

 public:
   /**
    * Constructor.
    * @param _type is the type of the interconnection
    * @param _live_variable is the set of variables crossing the connection
    */
   connection_obj(element_t _type, const CustomOrderedSet<data_transfer>& _live_variable)
       : type(_type), live_variable(_live_variable)
   {
   }

   virtual ~connection_obj() = default;

   /**
    * Returns the name associated with the element
    * @return a string containing the name associated to element.
    */
   virtual const std::string get_string() const = 0;

   /**
    * Gets the temporary set
    * @return the set of temporary that could cross the connection
    */
   const CustomOrderedSet<data_transfer>& get_variables() const
   {
      return live_variable;
   }

   /**
    * Returns type of object used to perform connection
    * @return an integer associated to object type
    */
   unsigned int get_type() const
   {
      return type;
   }
};

/// RefCount definition for connection_obj class
using connection_objRef = refcount<connection_obj>;

#endif
