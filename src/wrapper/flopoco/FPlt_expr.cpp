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
 * @file FPlt_expr.Cpp
 * @brief FPlt_expr module for flopoco.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Autoheader include
#include "config_SKIP_WARNING_SECTIONS.hpp"

#if SKIP_WARNING_SECTIONS
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wunused-parameter"
#endif

#include <cmath>
#include <cstring>
#include <iosfwd>
#include <sstream>
#include <vector>

#include <cstddef>
#include <gmp.h>

#include "utils.hpp"
#include <gmpxx.h>

#include "FPAdderSinglePath.hpp"
#include "FPlt_expr.hpp"

#include "custom_map.hpp"
#include <cmath>
#include <cstdlib>
#include <iomanip>
#include <list>
#include <locale>
#include <sstream>
#include <string>
#include <vector>

#include <cstdio>
#include <mpfr.h>

#include "flopoco_wrapper.hpp"

using namespace std;

namespace flopoco
{
   extern vector<Operator*> oplist;

#define DEBUGVHDL 0

   FPlt_expr::FPlt_expr(Target* _target, int wE, int wF) : Operator(_target)
   {
      ostringstream name;

      name << "FPlt_expr_" << wE << "_" << wF;
      setName(name.str());

      setCopyrightString("Fabrizio Ferrandi (2011-2018)");

      /* Set up the IO signals */

      addFPInput("X", wE, wF);
      addFPInput("Y", wE, wF);
      addOutput("R", 1);

      /*	VHDL code description	*/
      manageCriticalPath(_target->localWireDelay() + _target->lutDelay());
      vhdl << tab << declare("nX", wE + wF + 3) << "  <= X" << range(wE + wF + 2, wE + wF + 1) << " & not(X" << of(wE + wF) << ") & X" << range(wE + wF - 1, 0) << ";" << endl;
      FPAdderSinglePath* value_difference = new FPAdderSinglePath(_target, wE, wF, wE, wF, wE, wF);
      value_difference->changeName(getName() + "value_difference");
      oplist.push_back(value_difference);
      inPortMap(value_difference, "X", "Y");
      inPortMap(value_difference, "Y", "nX");
      outPortMap(value_difference, "R", "valueDiff");
      vhdl << instance(value_difference, "value_difference");
      syncCycleFromSignal("valueDiff");
      setCriticalPath(value_difference->getOutputDelay("R"));

      manageCriticalPath(_target->localWireDelay() + _target->lutDelay());
      vhdl << tab << "R(0) <= '1' when (valueDiff" << of(wE + wF) << "='0') and (valueDiff" << range(wE + wF + 2, wE + wF + 1) << " /= \"00\") else '0';" << endl;
   }

   FPlt_expr::~FPlt_expr() = default;

   void FPlt_expr::emulate(TestCase*)
   {
      // TODO
   }

   void FPlt_expr::buildStandardTestCases(TestCaseList*)
   {
   }

} // namespace flopoco
