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
 * @file weak_dominance.hpp
 * @brief Class specifying weak dominance calculus.
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#ifndef WEAK_DOMINANCE_HPP
#define WEAK_DOMINANCE_HPP

#include "custom_map.hpp"
#include "graph.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);

#define WD_SELECTOR 1

/**
 * Class used to compute weak dominance
 */
class weak_dominance
{
 private:
   /// the input graph (not const since node info are copied in the new graph
   const graph* input;

   /// the entry vertex
   vertex start;

   /// the exit vertex
   vertex end;

   /// the selector used in output graph
   const int selector;

   /// The set of input parameters
   const ParameterConstRef param;

   /// the debug level
   const int debug_level;

   /**
    * Add a weak dominance edge between two nodes
    * @param source is the source vertex
    * @param target is the target vertex
    * @param output is the graph
    */
   void add_edge(vertex source, vertex target, graphs_collection* output);

 public:
   /**
    * Constructor
    * @param _input is the input graph
    * @param _start is the start vertex
    * @param _end is the end vertex
    * @param _param is the set of input parameters
    * @param _selector is the selctor of edges to be added
    */
   weak_dominance(const graph* _input, vertex _start, vertex _end, const ParameterConstRef param, int selector = WD_SELECTOR);

   /**
    * Destructor
    */
   ~weak_dominance() = default;

   /**
    * Compute weak dominance info
    * @param output is the graph on which weak dominance edges have to be added;
    * @param i2o is the map from input vertex to output vertex
    * @param o2i is the map from output vertex to input vertex
    */
   void calculate_weak_dominance_info(graphs_collection* output, CustomUnorderedMap<vertex, vertex>& i2o, CustomUnorderedMap<vertex, vertex>& o2i);
};
#endif
