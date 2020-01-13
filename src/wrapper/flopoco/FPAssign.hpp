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
 * @file FPAssign.hpp
 * @brief FPAssign module for flopoco.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef FPAssign_HPP
#define FPAssign_HPP
#include <gmp.h>
#include <gmpxx.h>
#include <mpfr.h>
#include <sstream>
#include <vector>

#undef DEBUG

#include "Operator.hpp"

namespace flopoco
{
   /** The FPAssign class */
   class FPAssign : public Operator
   {
    public:
      /**
       * The  constructor
       * @param[in]		target		the target device
       * @param[in]		wER			the with of the exponent in input
       * @param[in]		wFR			the with of the fraction in input
       */
      FPAssign(Target* target, int wER, int wFR);

      /**
       *  destructor
       */
      ~FPAssign() override;

      void emulate(TestCase* tc) override;
      void buildStandardTestCases(TestCaseList* tcl) override;

    private:
   };
} // namespace flopoco
#endif
