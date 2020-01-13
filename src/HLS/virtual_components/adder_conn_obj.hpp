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
 * @file adder_conn_obj.hpp
 * @brief Class implementation of the adder connection module.
 *
 *
 * @author Mattia Zordan <mattia.zordan@gmail.com>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef ADDER_CONN_OBJ_HPP
#define ADDER_CONN_OBJ_HPP

#include "generic_obj.hpp"

/**
 * @class adder_conn_obj
 * This class is used when pointers arithmetic is implicitly performed
 */
class adder_conn_obj : public generic_obj
{
   /// number of bit of in/out ports
   unsigned int bitsize;

   /// when true the addition can trim the last bits
   bool is_aligned_adder_p;

   /// last bits that can be trimmed
   unsigned int trimmed_bits;

 public:
   /**
    * Constructor
    */
   adder_conn_obj(const std::string& _name) : generic_obj(ADDER_CONN_OBJ, _name), bitsize(0), is_aligned_adder_p(false), trimmed_bits(0)
   {
   }

   /**
    * Destructor.
    */
   ~adder_conn_obj() override = default;

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
   void set_trimmed_bits(unsigned int _trimmed_bits)
   {
      is_aligned_adder_p = true;
      trimmed_bits = _trimmed_bits;
   }

   /// return the trimmed bits
   unsigned int get_trimmed_bits() const
   {
      return trimmed_bits;
   }

   /// return true in case the addition is aligned
   bool is_align_adder() const
   {
      return is_aligned_adder_p;
   }
};

#endif // ADDER_CONN_OBJ_HPP
