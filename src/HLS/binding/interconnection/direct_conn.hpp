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
 * @file direct_conn.hpp
 * @brief Class adopted to represent direct connections inside the datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef _DIRECT_CONN_HPP_
#define _DIRECT_CONN_HPP_

#include "connection_obj.hpp"
#include "refcount.hpp"

/**
 * Class to represent direct connections
 */
class direct_conn : public connection_obj
{
 public:
   /**
    * Costructor.
    */
   explicit direct_conn(const CustomOrderedSet<data_transfer>& _live_variable) : connection_obj(DIRECT_CONN, _live_variable)
   {
   }

   /**
    * Destructor.
    */
   ~direct_conn() override = default;

   /**
    * Returns the name associated with the element
    * @return a string containing the identifier
    */
   const std::string get_string() const override
   {
      return "DIRECT_CONNECTION";
   }
};
/// refcount definition of the class
typedef refcount<direct_conn> direct_connRef;

#endif
