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
 * @file mux_conn.hpp
 * @brief Class adopt to represent a mux connection
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
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
   enum
   {
      MUX_T_PORT = 1,
      MUX_F_PORT = 2
   };
   /**
    * Constructor.
    * @param _live_variable is the set of data transfers
    * @param _mux_tree is mux tree for the new connection
    */
   mux_conn(const CustomOrderedSet<data_transfer>& _live_variable,
            const std::vector<std::pair<generic_objRef, unsigned int>>& _mux_tree)
       : connection_obj(BY_MUX, _live_variable), mux_tree(_mux_tree)
   {
   }

   /**
    * Returns the name associated with the element
    * @return a string containing the name associated with the element.
    */
   const std::string get_string() const override
   {
      THROW_ASSERT(mux_tree.size() > 0, "Mux connection without any multiplexer associated");
      return mux_tree[0].first->get_string() + (mux_tree[0].second == MUX_T_PORT ? "(T)" : "(F)");
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
using mux_connRef = refcount<mux_conn>;

#endif
