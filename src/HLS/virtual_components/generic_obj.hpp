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
 * @file generic_obj.hpp
 * @brief Base class for all resources into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef GEN_OBJECT_HPP
#define GEN_OBJECT_HPP

#include "refcount.hpp"

#include <string>
#include <utility>

#include "config_HAVE_UNORDERED.hpp"

REF_FORWARD_DECL(structural_object);

/**
 * @class generic_obj
 * Generic class managing all resources into datapath.
 */
class generic_obj
{
 public:
   /// Admissible resource types
   enum resource_type
   {
      FUNCTIONAL_UNIT = 0, /// a functional resource
      REGISTER,            /// a register
      MULTI_UNBOUNDED_OBJ, /// a multi unbounded controller
      COMMAND_PORT,        /// a command port (mainly connections from/to controller)
      DATA_PORT,           /// a data port (in/out data)
      CONNECTION_ELEMENT,  /// an element used for connecting the resources (e.g., muxes)
      ADDER_CONN_OBJ,      /// an adder object representation used to compute some addresses
      UU_CONV_CONN_OBJ,    /// a converter from unsigned to unsigned int
      UI_CONV_CONN_OBJ,    /// a converter from unsigned to signed int
      IU_CONV_CONN_OBJ,    /// a converter from signed to unsigned int
      II_CONV_CONN_OBJ,    /// a converter from signed to signed int
      FF_CONV_CONN_OBJ,    /// a converter from real to real int
      I_ASSIGN_CONN_OBJ,   /// specify the type of a connection object: INT
      U_ASSIGN_CONN_OBJ,   /// specify the type of a connection object: UINT
      F_ASSIGN_CONN_OBJ,   /// specify the type of a connection object: REAL
   };

 protected:
   /// type of resource
   const resource_type type;

   /// structural_object associated to element
   Wrefcount<structural_object> SM;

   /// output signal associated to element. It allows to connect multiple elements to output of this object.
   /// So broadcast communication is possible
   Wrefcount<structural_object> out_sign;

   /// connection obj id
   std::string name;

 public:
   /**
    * This is the constructor of the object class.
    */
   generic_obj(const resource_type t, const std::string& _name) : type(t), name(_name)
   {
   }

   virtual ~generic_obj() = default;

   /**
    * Prints elements into given stream
    * @param os is the selected stream
    */
   void print(std::ostream& os) const
   {
      os << name;
   }

   /**
    * Friend definition of the << operator.
    * @param os is output stream
    * @param s is object to be written
    * @return the stream where the object is written
    */
   friend std::ostream& operator<<(std::ostream& os, const generic_obj& s)
   {
      s.print(os);
      return os;
   }

   /**
    * Returns the name associated with the element
    * @return the name associated with element.
    */
   const std::string get_string() const
   {
      return name;
   }

   /**
    * Return generic_obj type
    * @sa generic_obj::resource_type to get more details on meaning of value
    * @return an integer representing resource type
    */
   unsigned int get_type() const
   {
      return type;
   }

   /**
    * Sets structural_object associated to this object
    * @param SM_ is reference to structural_object to be associated
    */
   void set_structural_obj(const structural_objectRef& SM_)
   {
      SM = SM_;
   }

   /**
    * Sets structural_object of output signal associated to this object
    * @param out_sign_ is reference to structural_object of signal to be associated
    */
   void set_out_sign(const structural_objectRef& out_sign_)
   {
      out_sign = out_sign_;
   }

   /**
    * Gets structural_object associated to this object
    * @return a reference to structural_object associated
    */
   const structural_objectRef get_structural_obj() const
   {
      return SM.lock();
   }

   /**
    * Gets structural_object of output signal associated to this object
    * @return a reference to structural_object of signal associated
    */
   const structural_objectRef get_out_sign() const
   {
      return out_sign.lock();
   }

   /**
    * @param other is the second operand
    * @return this < other
    */
   bool operator<(const generic_obj& other) const;
};

/// RefCount definition for generic_obj class
using generic_objRef = refcount<generic_obj>;

#if !HAVE_UNORDERED
class GenericObjSorter
{
 public:
   GenericObjSorter();

   /**
    * Compare position of generic objects
    * @param x is the first generic_obj
    * @param y is the second generic_obj
    * @return true if index of x is less than y
    */
   bool operator()(const generic_objRef& x, const generic_objRef& y) const;
};

class GenericObjUnsignedIntSorter

{
 public:
   GenericObjUnsignedIntSorter();

   /**
    * Compare position of generic object-ints
    * @param x is the first generic_obj-int
    * @param y is the second generic_obj-int
    * @return true if index of x is less than y
    */
   bool operator()(const std::pair<generic_objRef, int>& x, const std::pair<generic_objRef, int>& y) const;
};
#endif
#endif
