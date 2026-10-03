/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file EucalyptusParameter.hpp
 * @brief
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef EUCALYPTUSPARAMETER_HPP
#define EUCALYPTUSPARAMETER_HPP

#include "Parameter.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(EucalyptusParameter);

class EucalyptusParameter : public Parameter
{
 private:
   /**
    * Check the compatibility among the different parameters
    * and compute implicated parameters
    */
   void CheckParameters() override;

   /**
    * Sets the default values with respect to the tool
    */
   void SetDefaults() override;

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

 public:
   /**
    * Constructor
    * @param program_name is the name of the executable
    * @param argc is the number of arguments
    * @param argv is the array of arguments passed to program.
    */
   EucalyptusParameter(const std::string& program_name, int argc, char** const argv);

   /**
    * Execute parameter parsing
    */
   int Exec() override;
};

#endif
