/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
