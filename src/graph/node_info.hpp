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
 * @file node_info.hpp
 * @brief Base class description of data information associated with each node of a graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef NODE_INFO_HPP
#define NODE_INFO_HPP

#include <ostream>

struct NodeInfo
{
   virtual ~NodeInfo() = default;

   /**
    * Print the information associated with the node of the graph.
    * @param os is the output stream.
    * @param detail_level is the detail level
    */
   virtual void print(std::ostream& os, int detail_level = 0) const;

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
};

inline void NodeInfo::print(std::ostream&, int) const
{
}

#endif
