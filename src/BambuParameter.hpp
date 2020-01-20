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
 * @file BambuParameter.hpp
 * @brief
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef BAMBUPARAMETER_HPP
#define BAMBUPARAMETER_HPP

#include "Parameter.hpp"
#include "refcount.hpp"

/**
 * @name Forward Declarations
 */
//@{
REF_FORWARD_DECL(BambuParameter);
//@}

class BambuParameter : public Parameter
{
 private:
   /**
    * Check the compatibility among the different parameters
    * and compute implicated parameters
    */
   void CheckParameters() override;

   /**
    * add the library to the raw to be considered
    * @param lib is the name of the library.
    */
   void add_bambu_library(std::string lib);

   /**
    * Print the usage of this tool
    * @param os is the stream where the message has to be printed
    */
   void PrintHelp(std::ostream& os) const override;

   /**
    * Print the name of the program to be included in the header
    * @param os is the stream on which the program name has to be printed
    */
   void PrintProgramName(std::ostream& os) const override;

   /**
    * add the following GCC options: -fwhole-program -fno-ipa-cp -fno-ipa-cp-clone and -D'printf(fmt, ...)='
    * @param kill_printf when true the option -D'printf(fmt, ...)=' added otherwise only the "whole" program options are added
    */
   void add_experimental_setup_gcc_options(bool kill_printf);

 public:
   /**
    * Constructor
    * @param program_name is the name of the executable
    * @param argc is the number of arguments
    * @param argv is the array of arguments passed to program.
    */
   BambuParameter(const std::string& program_name, int argc, char** const argv);

   /**
    * Destructor
    */
   ~BambuParameter() override = default;

   /**
    * Execute parameter parsing
    */
   int Exec() override;

   /**
    * Sets default values
    */
   void SetDefaults() override;
};

typedef refcount<BambuParameter> BambuParameterRef;

#endif
