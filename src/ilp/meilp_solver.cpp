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
 * @file meilp_solver.cpp
 * @brief This class provide an interface to lp_solve and glpk solvers.
 *
 * This class provide an interface to lp_solve and glpk solvers.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

#include <utility>

#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "meilp_solver.hpp"

#if HAVE_GLPK
#include "glpk_solver.hpp"
#endif

#if HAVE_COIN_OR
#include "cbc_solver.hpp"
#endif

#if HAVE_LP_SOLVE
#include "lp_solve_solver.hpp"
#endif

meilp_solver::meilp_solver() : nel(0), real_buffer(nullptr), int_buffer(nullptr), unique_column_id(0), MAX_time(0), debug_level(DEBUG_LEVEL_NONE)
{
}

meilp_solver::~meilp_solver()
{
   if(nel)
   {
      delete[] real_buffer;
      delete[] int_buffer;
   }
}

void meilp_solver::resize(size_t count)
{
   if(count > nel)
   {
      delete[] real_buffer;
      delete[] int_buffer;
      THROW_ASSERT(count > 0, "expected a positive number of variables");
      real_buffer = new double[count];
      int_buffer = new int[count];
      nel = count;
   }
}

void meilp_solver::set_debug_level(int dl)
{
   debug_level = dl;
}

void meilp_solver::set_priority(const std::map<int, int>& _priority)
{
   priority = _priority;
}

void meilp_solver::copy(const std::map<int, double>& i_coeffs)
{
   resize(i_coeffs.size());
   auto i_end = i_coeffs.end();
   int index = 0;
   for(auto i = i_coeffs.begin(); i != i_end; ++i, index++)
   {
      this->real_buffer[index] = i->second;
      this->int_buffer[index] = i->first + 1;
   }
}

meilp_solverRef meilp_solver::create_solver(supported_solvers solver_type)
{
   switch(solver_type)
   {
#if HAVE_GLPK
      case GLPK:
         return meilp_solverRef(new glpk_solver());
#endif
#if HAVE_COIN_OR
      case COIN_OR:
         return meilp_solverRef(new cbc_solver());
#endif
#if HAVE_LP_SOLVE
      case LP_SOLVE:
         return meilp_solverRef(new lp_solve_solver());
#endif
      default:
         THROW_ERROR("not supported solver type");
   }
   // not reachable point
   return meilp_solverRef();
}

void meilp_solver::set_binary(int i)
{
   set_int(i);
   set_bnds(i, 0, 1);
}

void meilp_solver::set_bnds(int var, double lowbo, double upbo)
{
   lower_bounds[var] = lowbo;
   upper_bounds[var] = upbo;
}

void meilp_solver::set_lowbo(int var, double bound)
{
   lower_bounds[var] = bound;
}

void meilp_solver::set_upbo(int var, double bound)
{
   upper_bounds[var] = bound;
}
