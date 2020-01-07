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
 * @file mux_conn.hpp
 * @brief Class adopt to represent a mux connection
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef _MUX_CONN_HPP_
#define _MUX_CONN_HPP_

#include <utility>

#include "connection_obj.hpp"
#include "generic_obj.hpp"
#include "refcount.hpp"

#include "op_graph.hpp"

/**
 * @class mux_conn
 * This class is used to represent a connection through multiplexers
 */
class mux_conn : public connection_obj
{
   /// It's sequence of multiplexer inputs to drive the signal from the source element to the target one. Note that
   /// each vector element is a multiplexer, connected to the next one
   /// the unsigned int represents the mux input port
   std::vector<std::pair<generic_objRef, unsigned int>> mux_tree;

 public:
   /**
    * Constructor.
    * @param live_variable is the set of data transfers
    * @param _mux_tree is mux tree for the new connection
    */
   mux_conn(const CustomOrderedSet<data_transfer>& _live_variable, std::vector<std::pair<generic_objRef, unsigned int>> _mux_tree) : connection_obj(BY_MUX, _live_variable), mux_tree(std::move(_mux_tree))
   {
   }

   /**
    * Destructor.
    */
   ~mux_conn() override = default;

   /**
    * Returns the name associated with the element
    * @return a string containing the name associated with the element.
    */
   const std::string get_string() const override
   {
      THROW_ASSERT(mux_tree.size() > 0, "Mux connection without any multiplexer associated");
      return mux_tree[0].first->get_string() + (mux_tree[0].second == T_COND ? "(T)" : "(F)");
   }

   /**
    * Returns the mux tree associated with the connection.
    * @return a vector where each element is a multiplexer, along with the corresponding active input.
    */
   std::vector<std::pair<generic_objRef, unsigned int>> get_mux_tree() const
   {
      return mux_tree;
   }

   /**
    * Returns the number of multiplexers required to implement the connection.
    * @return an integer representing the number of multiplexers (i.e., the length of the tree)
    */
   unsigned int get_mux_tree_size() const
   {
      return static_cast<unsigned int>(mux_tree.size());
   }
};
/// refcount definition of the class
typedef refcount<mux_conn> mux_connRef;

#endif
