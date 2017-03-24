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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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
 * @file memory.hpp
 * @brief Datastructure to represent memory information in high-level synthesis
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
*/

#ifndef MEMORY_CS_H
#define MEMORY_CS_H
#include "memory.hpp"

class memory_CS : memory
{  
    /// bus data bitsize
    unsigned int bus_tag_bitsize;

    /// tag of memory assigned to each function
    std::map<int,int> tag_memory;

public:
   /**
    * Constructor
    */
   memory_CS(const tree_managerRef TreeM, unsigned int off_base_address, unsigned int max_bram, bool null_pointer_check, bool initial_internal_address_p, unsigned int initial_internal_address, unsigned int &_address_bitsize);

   /**
    * Destructor
    */
   ~memory_CS();

   /**
    * set the bus tag bitsize
    */
   void set_bus_tag_bitsize(unsigned int bitsize) {bus_tag_bitsize=bitsize;}

   /**
    * return the bitsize of the tag bus
    */
   unsigned int get_bus_tag_bitsize() const {return bus_tag_bitsize;}

   /**
    * set the bus tag bitsize
    */
   void set_tag_memory_number(int funID, int tag_number) {tag_memory[funID]=tag_number;}

   /**
    * return the bitsize of the tag bus
    */
   int get_tag_memory_number(int funID) {return tag_memory[funID];}

};

#endif // MEMORY_CS_H
