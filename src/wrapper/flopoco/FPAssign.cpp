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
 *              Copyright (c) 2004-2017 Politecnico di Milano
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

///Autoheader include
#include "config_SKIP_WARNING_SECTIONS.hpp"

#if SKIP_WARNING_SECTIONS
#pragma GCC diagnostic ignored "-Wshadow"
#pragma GCC diagnostic ignored "-Wsign-conversion"
#endif

#include <iosfwd>
#include <sstream>
#include <vector>
#include <math.h>
#include <string.h>

#include <cstddef>
#include <gmp.h>

#include <gmpxx.h>
#include "utils.hpp"

#include "FPAssign.hpp"

#include <cstdlib>
#include <iomanip>
#include <sstream>
#include <string>
#include <list>
#include <map>
#include <vector>
#include <math.h>
#include <locale>

#include <stdio.h>
#include <mpfr.h>

using namespace std;

namespace flopoco{

   extern vector<Operator*> oplist;

#define DEBUGVHDL 0


   FPAssign::FPAssign(Target* _target, int wE, int wF) :
         Operator(_target) {

      ostringstream name;

      name<<"FPAssign_" <<wE<<"_"<<wF;
      setName(name.str());

      setCopyrightString("Fabrizio Ferrandi (2011-2017)");

      /* Set up the IO signals */

      addFPInput ("X", wE,wF);
      addFPOutput ("R", wE,wF);

      /*	VHDL code description	*/
      vhdl << tab << "R <= X;"<<endl;
   }


   FPAssign::~FPAssign() {
   }


   void FPAssign::emulate(TestCase * )
   {
      // TODO
   }


   void FPAssign::buildStandardTestCases(TestCaseList* ){

   }

}
