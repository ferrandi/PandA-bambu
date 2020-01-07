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
 * @file Object.hpp
 * @brief Base class for all resources into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef CONN_ELEMENT_HPP
#define CONN_ELEMENT_HPP

#include <utility>

#include "conn_binding.hpp"
#include "refcount.hpp"
class graph;

/**
 * @class connection_obj
 * Generic class managing elements used to interconnect generic objects into datapath
 */
class connection_obj
{
 public:
   /// resource type
   typedef enum
   {
      DIRECT_CONN,
      BY_MUX
   } element_t;

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
   connection_obj(element_t _type, CustomOrderedSet<data_transfer> _live_variable) : type(_type), live_variable(std::move(_live_variable))
   {
   }

   /**
    * Destructor.
    */
   virtual ~connection_obj()
   {
   }

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
typedef refcount<connection_obj> connection_objRef;

#endif
