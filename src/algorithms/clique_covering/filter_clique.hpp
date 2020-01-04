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
 * @file filter_clique.hpp
 * @author Stefano Bodini, Federico Badini
 *
 */
#ifndef FILTER_CLIQUE_HPP
#define FILTER_CLIQUE_HPP

#include "clique_covering_graph.hpp"

/**
 * Functor used to reduce the size of clique: the rationale of filtering is
 * that too many sharing may create problem to the timing closure of the design.
 */
template <typename vertex_type>
struct filter_clique
{
   virtual ~filter_clique()
   {
   }

   virtual bool select_candidate_to_remove(const CustomOrderedSet<C_vertex>& candidate_clique, C_vertex& v, const std::map<C_vertex, vertex_type>& converter, const cc_compatibility_graph& cg) const = 0;
};

template <typename vertex_type>
struct no_filter_clique : public filter_clique<vertex_type>
{
   bool select_candidate_to_remove(const CustomOrderedSet<C_vertex>&, C_vertex&, const std::map<C_vertex, vertex_type>&, const cc_compatibility_graph&) const override
   {
      return false;
   }
};

#endif
