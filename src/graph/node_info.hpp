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
 * @file node_info.hpp
 * @brief Base class description of data information associated with each node of a graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef NODE_INFO_HPP
#define NODE_INFO_HPP

/// STD include
#include <ostream>

/// Utility include
#include "refcount.hpp"

REF_FORWARD_DECL(NodeInfo);

struct NodeInfo
{
   /// Constructor
   NodeInfo();

   /// Destructor
   virtual ~NodeInfo();

   /**
    * Print the information associated with the node of the graph.
    * @param os is the output stream.
    */
   virtual void print(std::ostream&, int detail_level = 0) const;

   /**
    * Friend definition of the << operator.
    * @param os is the output stream.
    * @param s is the node to print.
    */
   friend std::ostream& operator<<(std::ostream& os, const NodeInfo& s)
   {
      s.print(os);
      return os;
   }
   /**
    * Friend definition of the << operator. Pointer version.
    * @param os is the output stream.
    * @param s is the node to print.
    */
   friend std::ostream& operator<<(std::ostream& os, const NodeInfoRef& s)
   {
      if(s)
         s->print(os);
      return os;
   }
};

/**
 * RefCount type definition of the NodeInfo class structure
 */
typedef refcount<NodeInfo> NodeInfoRef;
#endif
