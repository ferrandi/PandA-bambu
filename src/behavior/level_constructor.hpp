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
 * @file level_constructor.hpp
 * @brief Data structore used to build the topological order of the operations vertices.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef LEVEL_CONSTRUCTOR_HPP
#define LEVEL_CONSTRUCTOR_HPP

#include "custom_map.hpp"
#include "graph.hpp" // for vertex
#include "refcount.hpp"
#include <deque>

/**
 * @name forward declarations
 */
//@{
REF_FORWARD_DECL(level_constructor);
//@}

/**
 * level manager.
 */
class level_constructor
{
 private:
   /**
    * Map vertex to position in topological order;
    * in the sorting then part vertices come before else part ones
    */
   std::map<vertex, unsigned int>& map_levels_true;

   /**
    * List of vertices sorted by topological order;
    * in the sorting then part vertices come before else part ones
    */
   std::deque<vertex>& deque_levels_true;

 public:
   /**
    * Constructor.
    * @param _map_levels_true is the reference to the map_levels_true
    * @param _deque_levels_true is the reference to the deque_levels_true
    */
   level_constructor(std::map<vertex, unsigned int>& _map_levels_true, std::deque<vertex>& _deque_levels_true);

   /**
    * Destructor.
    */
   ~level_constructor();

   /**
    * Add a vertex to the deque and to the map.
    */
   void add(vertex v, unsigned int index);
};

#endif
