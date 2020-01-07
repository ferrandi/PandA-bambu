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
 * @file meilp_solver.hpp
 * @brief This class provide an interface to different solvers. We will support lp_solve and glpk in near future.
 *
 * This class provide an interface to lp_solve and glpk solvers.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef MEILP_SOLVER_HPP
#define MEILP_SOLVER_HPP

/// Autoheader include
#include "config_HAVE_COIN_OR.hpp"
#include "config_HAVE_GLPK.hpp"
#include "config_HAVE_LP_SOLVE.hpp"

#include <cstddef>
#include <iosfwd>
#include <string>
#include <vector>

/// Utility include
#include "custom_map.hpp"
#include "refcount.hpp"

/**
 * @name forward declarations
 */
//@{
/// RefCount type definition of the meilp_solver class structure
REF_FORWARD_DECL(meilp_solver);
//@}

/**
 * Base class providing several methods to encapsulate the interface to several ilp solver.
 */
class meilp_solver
{
 public:
   /**
    * Possible operator in constraints
    */
   enum ilp_sign
   {
      G, /**< Greater then **/
      L, /**< Less then **/
      E  /**< Equal to **/
   };

 private:
   /// number of elements in the constraint buffer
   size_t nel;

 protected:
   /// values in the constraint buffer
   double* real_buffer;

   /// indexes in the constraint buffer
   int* int_buffer;

   /// unique column identifier
   int unique_column_id;

   /// Time-out value
   int MAX_time;

   /// debug_level
   int debug_level;

   /// variables priority
   std::map<int, int> priority;

   /// The lower bound of the variables. They will be really set by solve method
   CustomUnorderedMap<int, double> lower_bounds;

   /// The upper bound of the variables. They will be really set by solve method
   CustomUnorderedMap<int, double> upper_bounds;

   /**
    * Constructor
    */
   meilp_solver();

   /**
    *
    * @param count
    */
   void resize(size_t count);

   /**
    *
    * @param i_coeffs
    */
   virtual void copy(const std::map<int, double>& i_coeffs);

   /**
    * Set the lower and upper of the variables using lower_bounds and upper_bounds
    */
   virtual void set_all_bounds() = 0;

   /**
    * Print the problem
    * @param os is the stream on which problem has to be printed
    */
   virtual void print(std::ostream& os) = 0;

 public:
   /**
    * List of currently supported solvers.
    */
   enum supported_solvers
   {
#if HAVE_GLPK
      GLPK, /**< GLPK based solver (http://www.gnu.org/software/glpk) */
#endif
#if HAVE_COIN_OR
      COIN_OR, /**< COIN-OR based solver (http://www.coin-or.org/) */
#endif
#if HAVE_LP_SOLVE
      LP_SOLVE /**< LP_SOLVE based solver (http://tech.groups.yahoo.com/group/lp_solve/) */
#endif
   };

   /**
    * Type of objective function
    */
   enum ilp_dir
   {
      min, /**< Minimization **/
      max  /**< Maximization **/
   };

   /**
    * virtual destructor
    */
   virtual ~meilp_solver();

   /**
    * ???
    * @param nvars
    */
   virtual void make(int nvars) = 0;

   /**
    * Solve the linear problem
    */
   virtual int solve() = 0;

   /**
    * Solve the integer linear problem
    */
   virtual int solve_ilp() = 0;

   /**
    * Add a constraint to the ilp formulation
    * @param i_coeffs are the coefficients of the variables in the left part
    * @param i_rhs is the constant in the right part
    * @param i_sign is the operator in the constraints
    * @param name is the name of the constraint
    */
   virtual void add_row(std::map<int, double>& i_coeffs, double i_rhs, ilp_sign i_sign, const std::string& name) = 0;

   /**
    * Set the objective function
    * @param I_coeffs are the coefficients of the variables in the objective function
    * @param dir is the type of objective function
    */
   virtual void objective_add(std::map<int, double>& i_coeffs, ilp_dir dir) = 0;

   /**
    * Set a variable to have only binary values
    * @param i is the variable
    */
   virtual void set_binary(int i);

   /**
    * Set a variable to have only integer values
    * @param i is the variables
    */
   virtual void set_int(int i) = 0;

   /**
    * Set lower and upper bound of a variable
    * @param var is the variables
    * @param lowbo is the lower bound
    * @param upbo is the upper bound
    */
   virtual void set_bnds(int var, double lowbo, double upbo);

   /**
    * Set lower bound of a variable
    * @param var is the variable
    * @param bound is the lower bound
    */
   virtual void set_lowbo(int var, double bound);

   /**
    * Set upper bound of a variable
    * @param var is the variable
    * @param bound is the upper bound
    */
   virtual void set_upbo(int var, double bound);

   /**
    * Return the solution of the problem
    * @param vars is where solution are stored
    */
   virtual void get_vars_solution(std::map<int, double>& vars) const = 0;

   /**
    * Return the number of constraints
    * @return the number of constraints
    */
   virtual int get_number_constraints() const = 0;

   /**
    * Return the number of variables
    * @return the number of variables
    */
   virtual int get_number_variables() const = 0;

   /**
    * Set name of a variable (column)
    * @param var is the index of the variables
    * @param name is the name of the variable
    */
   virtual void set_col_name(int var, const std::string& name) = 0;

   /**
    * Get name of a variable (column)
    * @param var is the index of the variables
    * return the name of the variable
    */
   virtual std::string get_col_name(int var) = 0;

   /**
    * Add an empty column ???
    */
   virtual int add_empty_column() = 0;

   /**
    * ???
    */
   void setMaximumSeconds(int MAX_t)
   {
      MAX_time = MAX_t;
   }

   /**
    * Set the verbosity (debug_level)
    */
   void set_debug_level(int v);

   /**
    * Set the variable priority
    * @param _priority is mapping between index variable and priority
    */
   void set_priority(const std::map<int, int>& _priority);

   /**
    * Factory static member function.
    * Given the supported enum create a specialization of meilp_solver
    * @param solver_type is the solver.
    * @return the solver object.
    */
   static meilp_solverRef create_solver(supported_solvers solver_type);

   /**
    * Print the problem
    * @param file_name is the name of the file to be written
    */
   virtual void print_to_file(const std::string& file_name) = 0;
};

#endif
