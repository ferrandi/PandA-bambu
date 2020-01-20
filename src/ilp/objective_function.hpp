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
 * @file objective_function.hpp
 * @brief This file defines the objective function class.
 *
 * An objective function is a linear function with a direction of
 * optimization (minimization or maximization of the objective function).
 *
 * @author Livio Dalloro <dalloro@users.sourceforge.net>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef OBJECTIVE_FUNCTION_HPP
#define OBJECTIVE_FUNCTION_HPP

#include "refcount.hpp"
#include <iosfwd>

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(problem_dim);
REF_FORWARD_DECL(meilp_solver);
//@}

/**
 *
 */
class objective_function
{
 protected:
 public:
   /**
    * This method prints the class to the standard output stream.
    * @param os the output stream.
    */
   virtual void print(std::ostream&) const
   {
   }

   virtual void set_objective_function(meilp_solverRef const& MS, problem_dimRef const& p_dimension) = 0;

   /**
    * The class constructor.
    */
   objective_function();
   /**
    * The class Destructor.
    */
   virtual ~objective_function();
};

typedef refcount<objective_function> objective_functionRef;

#endif
