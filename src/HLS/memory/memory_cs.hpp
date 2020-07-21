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
 *              Copyright (c) 2016-2020 Politecnico di Milano
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
 * @file mem_dominator_allocation_CS.hpp
 * @brief add tag information
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
 */

#ifndef MEMORY_CS_H
#define MEMORY_CS_H
#include "memory.hpp"

class memory_cs : public memory
{
   /// bus data bitsize
   unsigned int bus_tag_bitsize;

 public:
   /**
    * Constructor
    */
   memory_cs(const tree_managerRef _TreeM, unsigned long long int _off_base_address, unsigned int max_bram, bool _null_pointer_check, bool initial_internal_address_p, unsigned int initial_internal_address, const unsigned int _address_bitsize)
       : memory(_TreeM, _off_base_address, max_bram, _null_pointer_check, initial_internal_address_p, initial_internal_address, _address_bitsize), bus_tag_bitsize(0)
   {
   }

   /**
    * Destructor
    */
   virtual ~memory_cs() = default;

   /**
    * set the bus tag bitsize
    */
   void set_bus_tag_bitsize(unsigned int bitsize)
   {
      bus_tag_bitsize = bitsize;
   }

   /**
    * return the bitsize of the tag bus
    */
   unsigned int get_bus_tag_bitsize() const
   {
      return bus_tag_bitsize;
   }
};

#endif // MEMORY_CS_H
