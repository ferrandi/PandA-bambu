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
 * @file funit_obj.hpp
 * @brief Base class for all register into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef FUNIT_HPP
#define FUNIT_HPP

#include <boost/lexical_cast.hpp>

#include "generic_obj.hpp"
#include "refcount.hpp"

/**
 * @class funit_obj
 * Class representing functional units in the datapath
 */
class funit_obj : public generic_obj
{
   /// instance number of the unit
   unsigned int index;
   /// type of the functional unit
   unsigned int fu_type;

   // operation selector list (first: operation name, second: datapath selector)
   std::map<std::string, generic_objRef> fuOpSelectors;

 public:
   /**
    * This is the constructor of the object class, with a given id
    * @param _name is the id
    * @param _index is the instance of the functional unit
    * @param _type is the functional unit type
    */
   funit_obj(const std::string& _name, unsigned int _type, unsigned int _index) : generic_obj(FUNCTIONAL_UNIT, _name), index(_index), fu_type(_type)
   {
   }

   /**
    * Destructor.
    */
   ~funit_obj() override = default;

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
   void add_selector_op(const generic_objRef& new_sel, std::string op_name)
   {
      fuOpSelectors.insert(std::make_pair(op_name, new_sel));
   }
   /**
    * Get selector object
    * @param op_name operation name (e.g. LOAD, STORE,..)
    * @return the index of selected funit
    */
   generic_objRef GetSelector_op(std::string op_name)
   {
      return fuOpSelectors[op_name];
   }
};

typedef refcount<funit_obj> funit_objRef;

#endif
