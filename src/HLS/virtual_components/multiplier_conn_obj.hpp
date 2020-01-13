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
 * @file multiplier_conn_obj.hpp
 * @brief Class implementation of the multiplier connection module.
 *
 *
 * @author Mattia Zordan <mattia.zordan@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef MULTIPLIER_CONN_OBJ_HPP
#define MULTIPLIER_CONN_OBJ_HPP

#include "generic_obj.hpp"

/**
 * @class multiplier_conn_obj
 * This class is used when pointers arithmetic is implicitly performed
 */
class multiplier_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

   /// when true the multiplication is a multiplication to a constant
   bool multiply_by_constant_p;

   /// constant value operand
   unsigned int constant_value;

 public:
   /**
    * Constructor
    */
   multiplier_conn_obj(const std::string& _name) : generic_obj(MULTIPLIER_CONN_OBJ, _name), bitsize(0), multiply_by_constant_p(false), constant_value(0)
   {
   }

   /**
    * Destructor.
    */
   ~multiplier_conn_obj() override = default;

   /**
    * add a size to the component
    */
   void add_bitsize(unsigned int _bitsize)
   {
      bitsize = _bitsize > bitsize ? _bitsize : bitsize;
   }

   /**
    * return the maximum bitsize associated with the component
    */
   unsigned int get_bitsize() const
   {
      return bitsize;
   }

   /// set the multiplication as a multiplication to a constant
   void set_multiplication_to_constant(unsigned int _constant_value)
   {
      multiply_by_constant_p = true;
      constant_value = _constant_value;
   }

   /// return the constant value
   unsigned int get_constant_value() const
   {
      return constant_value;
   }

   /// return true in case of multiplication to a constant
   bool is_multiplication_to_constant() const
   {
      return multiply_by_constant_p;
   }
};

#endif // MULTIPLIER_CONN_OBJ_HPP
