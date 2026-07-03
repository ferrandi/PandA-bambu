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
 * @file funit_obj.hpp
 * @brief Base class for all register into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef FUNIT_HPP
#define FUNIT_HPP

#include "generic_obj.hpp"
#include "refcount.hpp"

/**
 * @class funit_obj
 * Class representing functional units in the datapath
 */
class funit_obj : public generic_obj
{
   /// instance number of the unit
   const unsigned int index;
   /// type of the functional unit
   const unsigned int fu_type;

   // operation selector list (first: operation name, second: datapath selector)
   std::map<std::string, generic_objRef> fuOpSelectors;

 public:
   /**
    * This is the constructor of the object class, with a given id
    * @param _name is the id
    * @param _index is the instance of the functional unit
    * @param _type is the functional unit type
    */
   funit_obj(const std::string& _name, unsigned int _type, unsigned int _index)
       : generic_obj(FUNCTIONAL_UNIT, _name), index(_index), fu_type(_type)
   {
   }

   /**
    * Get funit name
    * @return the name of selected funit
    */
   unsigned int get_fu() const
   {
      return fu_type;
   }

   /**
    * Get funit index
    * @return the index of selected funit
    */
   unsigned int get_index() const
   {
      return index;
   }

   /**
    * Add selector to list
    * @param new_sel is the selector
    * @param op_name is the operation name
    */
   void add_selector_op(const generic_objRef& new_sel, const std::string& op_name)
   {
      fuOpSelectors.insert(std::make_pair(op_name, new_sel));
   }
   /**
    * Get selector object
    * @param op_name operation name (e.g. LOAD, STORE,..)
    * @return the index of selected funit
    */
   generic_objRef GetSelector_op(const std::string& op_name)
   {
      return fuOpSelectors[op_name];
   }
};

using funit_objRef = refcount<funit_obj>;

#endif
