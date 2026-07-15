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
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file interference_graph.hpp
 * @brief This header file define a simple and efficient interference graph
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef INTERFERENCE_GRAPH_HPP
#define INTERFERENCE_GRAPH_HPP

#include "custom_set.hpp"

class interferenceGraphClass
{
 public:
   using key_type = std::pair<unsigned, unsigned>;
   using set_type = CustomUnorderedSet<key_type>;

   interferenceGraphClass()
   {
   }

   void add_edge(unsigned i, unsigned j)
   {
      ensure_order(i, j);
      data_.emplace(i, j);
   }

   bool operator()(unsigned i, unsigned j) const
   {
      ensure_order(i, j);
      return data_.find({i, j}) != data_.end();
   }

 private:
   set_type data_;

   static void ensure_order(unsigned& i, unsigned& j)
   {
      if(i > j)
      {
         std::swap(i, j);
      }
   }
};

#endif // INTERFERENCE_GRAPH_HPP
