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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * @file FPAssign.cpp
 * @brief FPAssign module for flopoco.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#include "FPAssign.hpp"

#include "custom_map.hpp"
#include "utils.hpp"

#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <gmp.h>
#include <gmpxx.h>
#include <iomanip>
#include <iosfwd>
#include <list>
#include <locale>
#include <mpfr.h>
#include <sstream>
#include <string>
#include <vector>

using namespace std;

namespace flopoco
{
   extern vector<Operator*> oplist;

#define DEBUGVHDL 0

   FPAssign::FPAssign(Target* _target, int wE, int wF) : Operator(_target)
   {
      ostringstream name;

      name << "FPAssign_" << wE << "_" << wF;
      setName(name.str());

      setCopyrightString("Fabrizio Ferrandi (2011-2018)");

      /* Set up the IO signals */

      addFPInput("X", wE, wF);
      addFPOutput("R", wE, wF);

      /*	VHDL code description	*/
      vhdl << tab << "R <= X;" << endl;
   }

   FPAssign::~FPAssign() = default;

   void FPAssign::emulate(TestCase*)
   {
      // TODO
   }

   void FPAssign::buildStandardTestCases(TestCaseList*)
   {
   }

} // namespace flopoco
