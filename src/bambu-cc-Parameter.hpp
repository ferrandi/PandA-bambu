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
 * @file bambu-cc-Parameter.hpp
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef BAMBU_CC_PARAMETER_HPP
#define BAMBU_CC_PARAMETER_HPP

/// Superclass include
#include "Parameter.hpp"

/// Utility include
#include "refcount.hpp"

class bambu_cc_parameter : public Parameter
{
 private:
   /**
    * Checks parameter values to set implicit one
    */
   void CheckParameters() override;

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
   bambu_cc_parameter(const std::string& program_name, int argc, char** const argv);

   /**
    * Execute parameter parsing
    */
   int Exec() override;

   /**
    * Sets the default values for the bambu-cc tool
    */
   void SetDefaults() override;
};

#endif
