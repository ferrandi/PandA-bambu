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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file BambuParameter.hpp
 * @brief
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef BAMBUPARAMETER_HPP
#define BAMBUPARAMETER_HPP

#include "Parameter.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(BambuParameter);

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
   void add_bambu_library(const std::string& lib);

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
    * @param kill_printf when true the option -D'printf(fmt, ...)=' added otherwise only the "whole" program options are
    * added
    */
   void add_experimental_setup_compiler_options(bool kill_printf);

 public:
   /**
    * Constructor
    * @param program_name is the name of the executable
    * @param argc is the number of arguments
    * @param argv is the array of arguments passed to program.
    */
   BambuParameter(const std::string& program_name, int argc, char** const argv);

   /**
    * Execute parameter parsing
    */
   int Exec() override;

   /**
    * Sets default values
    */
   void SetDefaults() override;
};

#endif
