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
 * @file dataport_obj.hpp
 * @brief Base class for all dataports into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef DATAPORT_OBJ_HPP
#define DATAPORT_OBJ_HPP

#include <utility>

#include "generic_obj.hpp"

/**
 * primary ports of datapath.
 */
class dataport_obj : public generic_obj
{
   /// define the parameter name of the object
   std::string parameter;

   /// number of bit
   unsigned int bitsize;

 public:
   /**
    * Constructor
    */
   dataport_obj(const std::string& _name, unsigned int _bitsize) : generic_obj(DATA_PORT, _name), bitsize(_bitsize)
   {
   }

   /**
    * Constructor
    */
   dataport_obj(const std::string& _name, std::string _parameter, unsigned int _bitsize) : generic_obj(DATA_PORT, _name), parameter(std::move(_parameter)), bitsize(_bitsize)
   {
   }

   /**
    * Destructor.
    */
   ~dataport_obj() override = default;

   /**
    * return the maximum bitsize associated with the component
    */
   unsigned int get_bitsize() const
   {
      return bitsize;
   }
};

#endif
