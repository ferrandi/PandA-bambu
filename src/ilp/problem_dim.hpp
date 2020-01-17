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
 * @file problem_dim.hpp
 * @brief This file defines the problem_dim struct. This
 * struct represents the dimension of the scheduling problem.
 *
 * It simply defines the number of operation of the specification, the
 * number of control steps, the number of functional unit types and
 * the number of branching blocks.
 *
 * @author Livio Dalloro <dalloro@users.sourceforge.net>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef PROBLEM_DIM_HPP
#define PROBLEM_DIM_HPP

#include "refcount.hpp"
#include <iosfwd>

/**
 * @name Forward declarations.
 */
//@{

REF_FORWARD_DECL(meilp_solver);
//@}

class problem_dim
{
 protected:
   /** store the ilp_solver. Used to set the type of the column. X variables are binary, while Z and W are integers*/
   const meilp_solverRef MS;

 public:
   explicit problem_dim(meilp_solverRef ms);

   virtual void print(std::ostream& os) const = 0;

   friend std::ostream& operator<<(std::ostream& os, const problem_dim& pd)
   {
      pd.print(os);
      return os;
   }

   virtual ~problem_dim();
};

typedef refcount<problem_dim> problem_dimRef;

#endif
