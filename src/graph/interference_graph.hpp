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
 *              Copyright (C) 2025-2026 Politecnico di Milano
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
