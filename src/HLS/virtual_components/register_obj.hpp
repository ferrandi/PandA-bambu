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
 * @file register_obj.hpp
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

#ifndef REGISTER_HPP
#define REGISTER_HPP

#include "generic_obj.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"

/**
 * class modeling a register object
 */
class register_obj : public generic_obj
{
   generic_objRef wr_enable;

 private:
   unsigned int register_index;

 public:
   /**
    * This is the constructor of the object class, with a given id
    * @param new_value is the new value for register entry
    */
   explicit register_obj(const unsigned int index) : generic_obj(REGISTER, std::string("reg_") + STR(index))
   {
      register_index = index;
   }

   /**
    * Destructor.
    */
   ~register_obj() override = default;

   /**
    * Gets the write enable object for the given register
    * @return a set of sets where each of them can enable register write (when all conditions contained are
    *        true)
    */
   generic_objRef get_wr_enable() const
   {
      return wr_enable;
   }

   /**
    * Sets the write enable for given register
    */
   void set_wr_enable(const generic_objRef& wr_en)
   {
      wr_enable = wr_en;
   }

   /**
    * Gets the index of the register represented by this object
    * @return the index of the represented register
    */
   unsigned int get_register_index()
   {
      return register_index;
   }
};

/// RefCount definition for register_obj class
typedef refcount<register_obj> register_objRef;

#endif
