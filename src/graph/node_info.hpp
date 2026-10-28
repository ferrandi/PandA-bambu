/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
