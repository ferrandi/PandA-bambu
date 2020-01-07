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
 * @file glpk_solver.cpp
 * @brief Linear Programming solver according to the newer syntax (from version 4.35)
 * of the GLPK solver
 *
 *
 * @author Luca De Marco <lucademarco@hotmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include "custom_map.hpp"
#include <boost/lexical_cast.hpp>
#include <cfloat>
#include <climits>
#include <iostream>
#include <utility>

#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "glpk_solver.hpp"
#include "meilp_solver.hpp"
#include "utility.hpp"

extern "C"
{
#define _GLP_PROB
#define GLP_PROB
#define GLP_PROB_DEFINED
#if HAVE_GLPK_NESTED
#include <glpk/glpk.h>
#else
#include <glpk.h>
#endif
}

glpk_solver::glpk_solver() : meilp_solver(), lp(nullptr), scp(nullptr), iocp(nullptr), bfcp(nullptr), mip_solution(false)
{
}

glpk_solver::~glpk_solver()
{
   if(lp)
   {
      glp_delete_prob(lp);
   }
   delete scp;
   delete iocp;
   delete bfcp;
}

/*
 * Will be passed as a callback pointer to the GLPK library. The info pointer is used to point to the verbosity field of the current glpk_solver instance.
 */
int glpk_print_hook(void* DEBUG_PARAMETER(info), const char* DEBUG_PARAMETER(msg))
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERBOSE, *(static_cast<int*>(info)), msg);
   // return non-zero so that GLPK does not print stuff on its own.
   return 1;
}

void glpk_solver::make(int nvars)
{
   if(lp)
   {
      glp_delete_prob(lp);
   }
   lp = glp_create_prob();

   delete bfcp;
   bfcp = new glp_bfcp();

   glp_get_bfcp(lp, bfcp);

   delete scp;
   scp = new glp_smcp();
   glp_init_smcp(scp);
   scp->presolve = GLP_ON;

   delete iocp;
   iocp = new glp_iocp();
   glp_init_iocp(iocp);
   iocp->presolve = GLP_ON;

   if(debug_level <= DEBUG_LEVEL_MINIMUM)
   {
      bfcp->msg_lev = iocp->msg_lev = scp->msg_lev = GLP_MSG_OFF;
   }
   else if(debug_level > DEBUG_LEVEL_MINIMUM)
   {
      bfcp->msg_lev = iocp->msg_lev = scp->msg_lev = GLP_MSG_ERR;
   }
   else if(debug_level > DEBUG_LEVEL_PEDANTIC)
   {
      bfcp->msg_lev = iocp->msg_lev = scp->msg_lev = GLP_MSG_ALL;
   }

   glp_set_bfcp(lp, bfcp);

   // set up the print hook so that output from GLPK goes through our own handler.
   // lib_term_hook ( &glpk_print_hook, (void *) &debug_level);

   if(lp == nullptr)
   {
      THROW_ERROR(std::string("ErrorLpSolve"));
   }
   if(nvars)
   {
      glp_add_cols(lp, nvars);
   }
   if(MAX_time > 0)
   {
      scp->tm_lim = MAX_time;
      scp->it_lim = MAX_time;
   }
}

int glpk_solver::solve()
{
   /// Setting the bounds
   set_all_bounds();
   mip_solution = false;
   /// Printing the problem
   if(debug_level >= DEBUG_LEVEL_VERBOSE)
   {
      print(std::cerr);
   }

   scp->meth = GLP_DUALP;        // use two-phase dual simplex, and if it fails, switch to the primal simplex
   scp->pricing = GLP_PT_STD;    // Pricing technique, standard (textbook)
   scp->r_test = GLP_RT_HAR;     // Ratio test technique: Harris’ two-pass ratio test
   scp->presolve = GLP_ON;       // use built in presolver
   scp->tol_bnd = double(1e-7);  // Tolerance used to check if the basic solution is primal feasible
   scp->tol_dj = double(1e-7);   // Tolerance used to check if the basic solution is dual feasible
   scp->tol_piv = double(1e-10); // Tolerance used to choose eligble pivotal elements of the simplex table
   scp->obj_ll = DBL_MIN;        // Lower limit of the objective function
   scp->obj_ul = DBL_MAX;        // Upper limit of the objective function
   scp->it_lim = INT_MAX;        // Simplex iteration limit
   scp->tm_lim = INT_MAX;        // Searching time limit, in milliseconds
   scp->out_frq = 200;           // Output frequency, in iterations. This parameter specifies how frequently the solver sends information about the solution process to the terminal
   scp->out_dly = 0;             // Output delay, in milliseconds
   scp->presolve = GLP_OFF;      // Enable the built-in presolver

   int simplex_res = glp_simplex(lp, scp);

   /* POSSIBLE RESULTS
   0          The LP problem instance has been successfully solved.
              (This code does not necessarily mean that the solver has
              found optimal solution. It only means that the solution
              process was successful.)
   GLP_EBADB  Unable to start the search, because the initial basis speci-
              fied in the problem object is invalid—the number of basic
              (auxiliary and structural) variables is not the same as the
              number of rows in the problem object.
   GLP_ESING  Unable to start the search, because the basis matrix corre-
              sponding to the initial basis is singular within the working
              precision.
   GLP_ECOND  Unable to start the search, because the basis matrix cor-
              responding to the initial basis is ill-conditioned, i.e. its
              condition number is too large.
   GLP_EBOUND Unable to start the search, because some double-bounded
              (auxiliary or structural) variables have incorrect bounds.
   GLP_EFAIL  The search was prematurely terminated due to the solver
              failure.
   GLP_EOBJLL The search was prematurely terminated, because the ob-
              jective function being maximized has reached its lower
              limit and continues decreasing (the dual simplex only).
   GLP_EOBJUL The search was prematurely terminated, because the ob-
              jective function being minimized has reached its upper
              limit and continues increasing (the dual simplex only).
   GLP_EITLIM The search was prematurely terminated, because the sim-
              plex iteration limit has been exceeded.
   GLP_ETMLIM The search was prematurely terminated, because the time
              limit has been exceeded.
   GLP_ENOPFS The LP problem instance has no primal feasible solution
              (only if the LP presolver is used).
   GLP_ENODFS The LP problem instance has no dual feasible solution
              (only if the LP presolver is used).
   */

   switch(simplex_res)
   {
      case 0:
      {
         PRINT_MSG("*** SUCCESS! The LP problem instance has been successfully solved! ***");
         break;
      }
      case GLP_EBADB:
      {
         PRINT_MSG("ERROR: Unable to start the search, because the initial basis specified in the problem"
                   " object is invalid the number of basic (auxiliary and structural) variables is not the"
                   " same as the number of rows in the problem object");
         break;
      }
      case GLP_ESING:
      {
         PRINT_MSG("ERROR: Unable to start the search, because the basis matrix corresponding to the initial basis"
                   " is singular within the working precision");
         break;
      }
      case GLP_ECOND:
      {
         PRINT_MSG("ERROR: Unable to start the search, because the basis matrix corresponding to the initial"
                   " basis is ill-conditioned, i.e. its condition number is too large");
         break;
      }
      case GLP_EBOUND:
      {
         PRINT_MSG("ERROR: Unable to start the search, because some double-bounded (auxiliary or structural)"
                   " variables have incorrect bounds");
         break;
      }
      case GLP_EFAIL:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated due to the solver failure");
         break;
      }
      case GLP_EOBJLL:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated, because the objective function being"
                   " maximized has reached its lower limit and continues decreasing (the dual simplex only)");
         break;
      }
      case GLP_EOBJUL:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated, because the objective function being"
                   " minimized has reached its upper limit and continues increasing (the dual simplex only)");
         break;
      }
      case GLP_EITLIM:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated, because the simplex iteration limit"
                   " has been exceeded");
         break;
      }
      case GLP_ETMLIM:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated, because the time limit has been exceeded");
         break;
      }
      case GLP_ENOPFS:
      {
         PRINT_MSG("The LP problem instance has no primal feasible solution (only if the LP presolver is used)");
         break;
      }
      case GLP_ENODFS:
      {
         PRINT_MSG("The LP problem instance has no dual feasible solution (only if the LP presolver is used)");
         break;
      }
      default:
         PRINT_MSG("ERROR: unknown glp_solve return code");
   }

   int status = glp_get_status(lp);

   /* POSSIBLE PROBLEM STATUS
   GLP_OPT    solution is optimal;
   GLP_FEAS   solution is feasible;
   GLP_INFEAS solution is infeasible;
   GLP_NOFEAS problem has no feasible solution;
   GLP_UNBND  problem has unbounded solution;
   GLP_UNDEF  solution is undefined.
    */

   switch(status)
   {
      case GLP_OPT:
      {
         PRINT_MSG("SUCCESS: PROBLEM SOLUTION IS OPTIMAL");
         break;
      }
      case GLP_FEAS:
      {
         PRINT_MSG("SUCCESS: PROBLEM SOLUTION IS FEASIBLE");
         break;
      }
      case GLP_INFEAS:
      {
         PRINT_MSG("FAILURE: PROBLEM SOLUTION IS INFEASIBLE");
         break;
      }
      case GLP_NOFEAS:
      {
         PRINT_MSG("FAILURE: PROBLEM HAS NO FEASIBLE SOLUTION");
         break;
      }
      case GLP_UNBND:
      {
         PRINT_MSG("FAILURE: PROBLEM HAS UNBOUNDED SOLUTION");
         break;
      }
      case GLP_UNDEF:
      {
         PRINT_MSG("FAILURE: PROBLEM SOLUTION IS UNDEFINED");
         break;
      }
      default:
         PRINT_MSG("ERROR: unknown glp_get_status return code");
   }

   int prim_stat = glp_get_prim_stat(lp);

   /* POSSIBLE PRIMAL STATUS
   GLP_UNDEF  primal solution is undefined;
   GLP_FEAS   primal solution is feasible;
   GLP_INFEAS primal solution is infeasible;
   GLP_NOFEAS no primal feasible solution exists.
   */

   switch(prim_stat)
   {
      case GLP_UNDEF:
      {
         PRINT_MSG("FAILURE: Primal solution is undefined");
         break;
      }
      case GLP_FEAS:
      {
         PRINT_MSG("SUCCESS: Primal solution is feasible");
         break;
      }
      case GLP_INFEAS:
      {
         PRINT_MSG("FAILURE: Primal solution is infeasible");
         break;
      }
      case GLP_NOFEAS:
      {
         PRINT_MSG("FAILURE: No Primal feasible solution exists");
         break;
      }
      default:
         PRINT_MSG("ERROR: unknown glp_get_prim_stat return code");
   }

   return (simplex_res == 0 && status == GLP_OPT && prim_stat == GLP_FEAS) ? 0 : 1;
}

int glpk_solver::solve_ilp()
{
   /// Setting the bounds
   set_all_bounds();
   mip_solution = true;

   /// Printing the problem
   if(debug_level >= DEBUG_LEVEL_VERBOSE)
   {
      print(std::cerr);
   }

   /* scale the problem data*/
   if(!iocp->presolve)
   {
      glp_scale_prob(lp, GLP_SF_AUTO);
   }
   /* construct starting LP basis */
   if(!iocp->presolve)
   {
      glp_adv_basis(lp, 0);
   }
   if(!iocp->presolve)
   {
      glp_set_bfcp(lp, bfcp);
      int simplex_res = glp_simplex(lp, scp);
      switch(simplex_res)
      {
         case 0:
         {
            // PRINT_MSG("*** SUCCESS! The LP problem instance has been successfully solved! ***");
            break;
         }
         case GLP_EBADB:
         {
            PRINT_MSG("ERROR: Unable to start the search, because the initial basis specified in the problem"
                      " object is invalid the number of basic (auxiliary and structural) variables is not the"
                      " same as the number of rows in the problem object");
            break;
         }
         case GLP_ESING:
         {
            PRINT_MSG("ERROR: Unable to start the search, because the basis matrix corresponding to the initial basis"
                      " is singular within the working precision");
            break;
         }
         case GLP_ECOND:
         {
            PRINT_MSG("ERROR: Unable to start the search, because the basis matrix corresponding to the initial"
                      " basis is ill-conditioned, i.e. its condition number is too large");
            break;
         }
         case GLP_EBOUND:
         {
            PRINT_MSG("ERROR: Unable to start the search, because some double-bounded (auxiliary or structural)"
                      " variables have incorrect bounds");
            break;
         }
         case GLP_EFAIL:
         {
            PRINT_MSG("ERROR: The search was prematurely terminated due to the solver failure");
            break;
         }
         case GLP_EOBJLL:
         {
            PRINT_MSG("ERROR: The search was prematurely terminated, because the objective function being"
                      " maximized has reached its lower limit and continues decreasing (the dual simplex only)");
            break;
         }
         case GLP_EOBJUL:
         {
            PRINT_MSG("ERROR: The search was prematurely terminated, because the objective function being"
                      " minimized has reached its upper limit and continues increasing (the dual simplex only)");
            break;
         }
         case GLP_EITLIM:
         {
            PRINT_MSG("ERROR: The search was prematurely terminated, because the simplex iteration limit"
                      " has been exceeded");
            break;
         }
         case GLP_ETMLIM:
         {
            PRINT_MSG("ERROR: The search was prematurely terminated, because the time limit has been exceeded");
            break;
         }
         case GLP_ENOPFS:
         {
            // PRINT_MSG("The LP problem instance has no primal feasible solution (only if the LP presolver is used)");
            return 1;
         }
         case GLP_ENODFS:
         {
            // PRINT_MSG("The LP problem instance has no dual feasible solution (only if the LP presolver is used)");
            return 1;
         }
         default:
            PRINT_MSG("ERROR: unknown glp_solve return code");
      }
   }
   int intopt_res = glp_intopt(lp, iocp);

   /* POSSIBLE RESULTS
   0           The LP problem instance has been successfully solved.
               (This code does not necessarily mean that the solver has
               found optimal solution. It only means that the solution
               process was successful.)
   GLP_EBOUND  Unable to start the search, because some double-bounded
               (auxiliary or structural) variables have incorrect bounds.
   GLP_EROOT   Unable to start the search, because optimal basis for initial
               LP relaxation is not provided. (This code may appear only
               if the presolver is disabled.)
   GLP_ENOPFS  Unable to start the search, because LP relaxation of the
               MIP problem instance has no primal feasible solution.
               (This code may appear only if the presolver is enabled.)
   GLP_ENODFS  Unable to start the search, because LP relaxation of the
               MIP problem instance has no dual feasible solution. In
               other word, this code means that if the LP relaxation has
               at least one primal feasible solution, its optimal solution is
               unbounded, so if the MIP problem has at least one integer
               feasible solution, its (integer) optimal solution is also un-
               bounded. (This code may appear only if the presolver is
               enabled.)
   GLP_EFAIL   The search was prematurely terminated due to the solver
               failure.
   GLP_EMIPGAP The search was prematurely terminated, because the rela-
               tive mip gap tolerance has been reached.
   GLP_ETMLIM  The search was prematurely terminated, because the time
               limit has been exceeded.
   GLP_ESTOP   The search was prematurely terminated by application.
               (This code may appear only if the advanced solver inter-
               face is used.)
   */

   switch(intopt_res)
   {
      case 0:
      {
         // PRINT_MSG("*** SUCCESS! The MILP problem instance has been successfully solved! ***");
         break;
      }
      case GLP_EBOUND:
      {
         PRINT_MSG("ERROR: Unable to start the search, because some double-bounded"
                   " (auxiliary or structural) variables have incorrect bounds");
         break;
      }
      case GLP_EROOT:
      {
         PRINT_MSG("ERROR: Unable to start the search, because optimal basis for initial"
                   " LP relaxation is not provided");
         break;
      }
      case GLP_ENOPFS:
      {
         /*PRINT_MSG("ERROR: Unable to start the search, because LP relaxation of the"
               " MIP problem instance has no primal feasible solution");*/
         return 1;
      }
      case GLP_ENODFS:
      {
         PRINT_MSG("Unable to start the search, because LP relaxation of the"
                   " MIP problem instance has no dual feasible solution. In"
                   " other word, this code means that if the LP relaxation has"
                   " at least one primal feasible solution, its optimal solution is"
                   " unbounded, so if the MIP problem has at least one integer"
                   " feasible solution, its (integer) optimal solution is also un"
                   " bounded.");
         break;
      }
      case GLP_EFAIL:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated due to the solver failure");
         break;
      }
      case GLP_EMIPGAP:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated, because the rela"
                   " tive mip gap tolerance has been reached");
         break;
      }
      case GLP_ETMLIM:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated, because the time limit has been exceeded");
         break;
      }
      case GLP_ESTOP:
      {
         PRINT_MSG("ERROR: The search was prematurely terminated by application");
         break;
      }
      default:
         PRINT_MSG("ERROR: unknown glp_intopt return code");
   }

   int status = glp_mip_status(lp);

   /* POSSIBLE PROBLEM STATUS
   GLP_OPT    solution is optimal;
   GLP_FEAS   solution is feasible;
   GLP_NOFEAS problem has no feasible solution;
   GLP_UNDEF  solution is undefined.
    */

   switch(status)
   {
      case GLP_OPT:
      {
         // PRINT_MSG("SUCCESS: PROBLEM SOLUTION IS OPTIMAL");
         break;
      }
      case GLP_FEAS:
      {
         PRINT_MSG("SUCCESS: PROBLEM SOLUTION IS FEASIBLE");
         break;
      }
      case GLP_NOFEAS:
      {
         PRINT_MSG("FAILURE: PROBLEM HAS NO FEASIBLE SOLUTION");
         break;
      }
      case GLP_UNDEF:
      {
         PRINT_MSG("FAILURE: PROBLEM SOLUTION IS UNDEFINED");
         break;
      }
      default:
         PRINT_MSG("ERROR: unknown glp_get_status return code");
   }
   return (intopt_res == 0 && status == GLP_OPT) ? 0 : 1;
}

void glpk_solver::add_row(std::map<int, double>& i_coeffs, double i_rhs, ilp_sign i_sign, const std::string& name)
{
   if(i_coeffs.empty())
   {
      return;
   }
   copy(i_coeffs);
   THROW_ASSERT(lp, "the matrix must exist");
   int row_index = glp_add_rows(lp, 1);
   if(name.length() < 255)
      glp_set_row_name(lp, row_index, const_cast<char*>(name.c_str()));
   else
      glp_set_row_name(lp, row_index, const_cast<char*>((name.substr(0, 252) + "...").c_str()));
   glp_set_mat_row(lp, row_index, static_cast<int>(i_coeffs.size()), int_buffer - 1, real_buffer - 1);
   switch(i_sign)
   {
      case E:
         glp_set_row_bnds(lp, row_index, GLP_FX, i_rhs, i_rhs);
         break;
      case L:
         glp_set_row_bnds(lp, row_index, GLP_UP, 0, i_rhs);
         break;
      case G:
         glp_set_row_bnds(lp, row_index, GLP_LO, i_rhs, 0); // we assume that the upper bound is ignored
         break;
      default:
         THROW_UNREACHABLE("");
   }
}

int glpk_solver::get_number_constraints() const
{
   return glp_get_num_rows(lp);
}

int glpk_solver::get_number_variables() const
{
   return glp_get_num_cols(lp);
}

void glpk_solver::objective_add(std::map<int, double>& i_coeffs, ilp_dir dir)
{
   THROW_ASSERT(lp, "the matrix must exist");
   switch(dir)
   {
      case min:
         glp_set_obj_dir(lp, GLP_MIN);
         break;
      case max:
         glp_set_obj_dir(lp, GLP_MAX);
         break;
      default:
         THROW_ERROR(std::string("ErrorLpSolve"));
   }

   auto i_end = i_coeffs.end();
   for(auto i = i_coeffs.begin(); i != i_end; ++i)
   {
      glp_set_obj_coef(lp, i->first + 1, i->second);
   }
}

void glpk_solver::print(std::ostream& os __attribute__((unused)))
{
   /// Setting the bounds
   set_all_bounds();
   // for the time being the os is not supported
   // set_outputstream(lp, os);
   glp_write_sol(lp, "glp_sol.txt");
   glp_write_mps(lp, GLP_MPS_FILE, nullptr, "glpk.mps");
   glp_write_lp(lp, nullptr, "glpk.lp");
}

void glpk_solver::print_to_file(const std::string& file_name)
{
   /// Setting the bounds
   set_all_bounds();
   // for the time being the os is not supported
   // set_outputstream(lp, os);
   glp_write_sol(lp, (file_name + ".txt").c_str());
   glp_write_mps(lp, GLP_MPS_FILE, nullptr, (file_name + ".mps").c_str());
   glp_write_lp(lp, nullptr, (file_name + ".lp").c_str());
}

void glpk_solver::set_int(int i)
{
   glp_set_col_kind(lp, i + 1, GLP_IV);
}

void glpk_solver::get_vars_solution(std::map<int, double>& vars) const
{
   vars.clear();
   int nc = glp_get_num_cols(lp);
   for(int i = 0; i < nc; i++)
   {
      if(mip_solution)
      {
         vars[i] = glp_mip_col_val(lp, i + 1);
      }
      else
      {
         vars[i] = glp_get_col_prim(lp, i + 1);
      }
   }
}

void glpk_solver::set_col_name(int var, const std::string& name)
{
   std::string ost = name + "_" + boost::lexical_cast<std::string>(var);
   glp_set_col_name(lp, var + 1, const_cast<char*>(ost.c_str()));
}

std::string glpk_solver::get_col_name(int var)
{
   return std::string(glp_get_col_name(lp, var + 1));
}

int glpk_solver::add_empty_column()
{
   return glp_add_cols(lp, 1) - 1;
}

void glpk_solver::set_all_bounds()
{
   /*   Type     Bounds    Comment
     GLP_FR −∞ < x < +∞  Free (unbounded) variable
     GLP_LO  lb ≤ x < +∞ Variable with lower bound
     GLP_UP −∞ < x ≤ ub  Variable with upper bound
     GLP_DB  lb ≤ x ≤ ub Double-bounded variable
     GLP_FX  lb = x = ub Fixed variable
     */

   // NOTE: when a variable is created, it's col_type is GLP_FX
   int nc = glp_get_num_cols(lp);
   for(int i = 0; i < nc; i++)
   {
      int var = i + 1;
      switch(glp_get_col_type(lp, var))
      {
         case GLP_FR:
            glp_set_col_bnds(lp, var, GLP_FR, 0, 0);
#if(__GNUC__ >= 7)
            [[gnu::fallthrough]];
#endif
         case GLP_LO:
         {
            if(lower_bounds.find(i) != lower_bounds.end())
            {
               glp_set_col_bnds(lp, var, GLP_LO, lower_bounds[i], 0);
            }
            else
            {
               THROW_ERROR("Lower bounded variable, but lower bound can't be found in lower_bounds");
            }
            break;
         }
         case GLP_FX:
         {
            // IF THE VARIABLE HAS A LOWER AND UPPER BOUND, IT MAY BECOME FX IF LB == UP or DB is UP > LB
            if((lower_bounds.find(i) != lower_bounds.end()) && (upper_bounds.find(i) != upper_bounds.end()))
            {
               if(upper_bounds[i] == lower_bounds[i])
               {
                  // PRINT_MSG("GLP_FX becomes GLP_FX");
                  glp_set_col_bnds(lp, var, GLP_FX, lower_bounds[i], upper_bounds[i]);
               }
               else
               {
                  // PRINT_MSG("GLP_FX becomes GLP_DB");
                  glp_set_col_bnds(lp, var, GLP_DB, lower_bounds[i], upper_bounds[i]);
               }
            }
            // IF THE VARIABLE HAS A LOWER BOUND, BUT DOES NOT HAVE AN UPPER BOUND IT BECOMES LOWER BOUNDED
            else if((lower_bounds.find(i) != lower_bounds.end()) && (upper_bounds.find(i) == upper_bounds.end()))
            {
               // PRINT_MSG("GLP_FX becomes GLP_LO");
               glp_set_col_bnds(lp, var, GLP_LO, lower_bounds[i], 0);
            }
            // IF THE VARIABLE HAS AN UPPER BOUND, BUT DOES NOT HAVE A LOWER BOUND IT BECOMES UPPER BOUNDED
            else if((lower_bounds.find(i) == lower_bounds.end()) && (upper_bounds.find(i) != upper_bounds.end()))
            {
               // PRINT_MSG("GLP_FX becomes GLP_UP");
               glp_set_col_bnds(lp, var, GLP_UP, 0, upper_bounds[i]);
            }
            else
            {
               THROW_ERROR("Fixed bounded variable, but either upper or lower bound can't be found in vectors or bounds are different");
            }
            break;
         }
         case GLP_UP:
         {
            if(upper_bounds.find(i) != upper_bounds.end())
            {
               glp_set_col_bnds(lp, var, GLP_UP, 0, upper_bounds[i]);
            }
            else
            {
               THROW_ERROR("Upper bounded variable, but upper bound can't be found in upper_bounds");
            }
            break;
         }
         case GLP_DB:
         {
            if((lower_bounds.find(i) != lower_bounds.end()) && (upper_bounds.find(i) != upper_bounds.end()))
            {
               THROW_ASSERT(lower_bounds[i] <= upper_bounds[i],
                            "Error in bound of variable " + boost::lexical_cast<std::string>(i) + " : " + boost::lexical_cast<std::string>(lower_bounds[i]) + " " + boost::lexical_cast<std::string>(glp_get_col_ub(lp, var)));
               glp_set_col_bnds(lp, var, GLP_DB, lower_bounds[i], upper_bounds[i]);
            }
            else
            {
               THROW_ERROR("Double bounded variable, but either lower or upper bound can't be found in bounds");
            }
            break;
         }
         default:
            THROW_ERROR(std::string("ErrorLpSolve"));
      }
   }
}
