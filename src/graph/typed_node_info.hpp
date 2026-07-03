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
 * @file typed_node_info.hpp
 * @brief Base class description of data information associated with each node of a graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */
#ifndef TYPED_NODE_INFO_HPP
#define TYPED_NODE_INFO_HPP
#include "node_info.hpp"

#include <ostream>

/**
 * constant string identifying an operation node of type entry. Used during the behavioral_manager building starting
 * from the compiler IR data structure.
 */
#define ENTRY "ENTRY"

/**
 * constant string identifying an operation node of type exit. Used during the behavioral_manager building starting from
 * the compiler IR data structure.
 */
#define EXIT "EXIT"

/**
 * constant identifying the node type of an entry node.
 */
#define TYPE_ENTRY 2

/**
 * constant identifying the node type of an exit node.
 */
#define TYPE_EXIT 4

/**
 * constant identifying the node type of a GENERIC operation.
 */
#define TYPE_GENERIC 8

/**
 * Base class storing user data information.
 * This class is associated with the graph data structure.
 */

struct TypedNodeInfo : public NodeInfo
{
   /**
    * Custom vertex property: node_operation.
    * This property defines which operation is performed: assignment, addition, comparison, etc.
    */
   std::string node_operation;

   /**
    * Definition of the node name property.
    */
   std::string vertex_name;

   /**
    * Custom vertex property: node_type.
    * This property defines which type of node is: read and write a port, read a constant, if, case, wait and notify or
    * generic operation.
    */
   unsigned int node_type{TYPE_GENERIC};

   virtual ~TypedNodeInfo() override = default;

   /**
    * Print the information associated with the node of the graph.
    * @param os is the output stream.
    * @param detail_level selects the amount of detail to print
    */
   void print(std::ostream& os, int detail_level = 0) const override;

   /**
    * Friend definition of the << operator.
    * @param os is the output stream.
    * @param s is the node to print.
    */
   friend std::ostream& operator<<(std::ostream& os, const TypedNodeInfo& s)
   {
      s.print(os);
      return os;
   }
};

inline void TypedNodeInfo::print(std::ostream& os, int) const
{
   os << node_type << " " << node_operation << " " << vertex_name << "\n";
}

#endif
