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
 * @file glpk_solver.hpp
 * @brief Linear Programming solver according to the newer syntax (from version 4.35)
 * of the GLPK solver
 *
 * Here goes a detailed description of the file
 *
 * @author Luca De Marco <lucademarco@hotmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef GLPK_SOLVER_HPP
#define GLPK_SOLVER_HPP

#include "custom_map.hpp"
#include "meilp_solver.hpp"
#include <iosfwd>
#include <string>

#include "config_HAVE_GLPK.hpp"
/// Autoheader include
#include "config_HAVE_GLPK_NESTED.hpp"
#if HAVE_GLPK_NESTED
#include <glpk/glpk.h>
#else
#include <glpk.h>
#endif

class glpk_solver : public meilp_solver
{
 private:
   glp_prob* lp;

   /**
    simplex_control_params
    */
   glp_smcp* scp;

   /**
    integer solver control parameter
    */
   glp_iocp* iocp;

   /**
    basis factorization control parameters
   */
   glp_bfcp* bfcp;

   /**
     it is true when solve_ilp() has been executed, false otherwise
    */
   bool mip_solution;
   /**
    * Set the lower and upper of the variables using lower_bounds and upper_bounds
    */
   void set_all_bounds() override;

   /**
    * Set the lower and upper of the variables using lower_bounds and upper_bounds
    */
   void set_all_bounds_new();

   /**
    * Print the problem
    * @param os is the stream on which problem has to be printed
    */
   void print(std::ostream& os) override;

 public:
   glpk_solver();
   ~glpk_solver() override;
   void make(int nvars) override;
   int solve() override;
   int solve_ilp() override;
   void add_row(std::map<int, double>& i_coeffs, double i_rhs, ilp_sign i_sign, const std::string& name) override;
   void objective_add(std::map<int, double>& i_coeffs, ilp_dir dir) override;

   void set_int(int i) override;
   void get_vars_solution(std::map<int, double>& vars) const override;
   int get_number_constraints() const override;
   int get_number_variables() const override;
   void set_col_name(int var, const std::string& name) override;

   /**
    * Get name of a variable (column)
    * @param var is the index of the variables
    * return the name of the variable
    */
   std::string get_col_name(int var) override;

   int add_empty_column() override;

   /**
    * Print the problem
    * @param file_name is the name of the file to be written
    */
   void print_to_file(const std::string& file_name) override;
};
#endif
