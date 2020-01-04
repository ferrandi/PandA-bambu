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
 * @file check_clique.hpp
 * @author Stefano Bodini, Federico Badini
 *
 */
#ifndef CHECK_CLIQUE_HPP
#define CHECK_CLIQUE_HPP

#include "clique_covering_graph.hpp"
#include "filter_clique.hpp"

template <typename vertex_type>
struct check_clique
{
   virtual ~check_clique()
   {
   }

   virtual bool check_edge_compatibility(C_vertex& rep, C_vertex& other) = 0;

   virtual bool check_no_mux_needed(C_vertex& rep, C_vertex& other) = 0;

   virtual check_clique* clone() const = 0;

   virtual double cost(size_t clique_count) = 0;

   virtual size_t num_mux() = 0;

   virtual void update_after_join(C_vertex&, C_vertex&) = 0;

   virtual void initialize_structures(boost_cc_compatibility_graph&, std::map<C_vertex, vertex_type>&) = 0;
};

template <typename vertex_type>
struct no_check_clique : public check_clique<vertex_type>
{
   bool check_edge_compatibility(C_vertex&, C_vertex&) override
   {
      return true;
   }
   bool check_no_mux_needed(C_vertex&, C_vertex&) override
   {
      return true;
   }
   no_check_clique* clone() const override
   {
      return new no_check_clique(*this);
   }
   double cost(size_t clique_count) override
   {
      return static_cast<double>(clique_count);
   }
   size_t num_mux() override
   {
      return 0;
   }
   void update_after_join(C_vertex&, C_vertex&) override
   {
   }
   void initialize_structures(boost_cc_compatibility_graph&, std::map<C_vertex, vertex_type>&) override
   {
   }
};

#endif
