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
 * @file parse_ir.cpp
 * @brief Implementation of the IR parsing interface function.
 *
 * Implementation of the function that parses IR from a file.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#include "parse_ir.hpp"

#include "Parameter.hpp"
#include "dbgPrintHelper.hpp"
#include "refcount.hpp"

#include <iostream>
#include <string>

// exit_code is stored in bambu.cpp
extern int exit_code;
extern ir_managerRef ir_parseY(const ParameterConstRef Param, std::string fn);

ir_managerRef ParseIRFile(const ParameterConstRef& Param, const std::string& f)
{
   try
   {
      return ir_parseY(Param, f);
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
   }
   THROW_ERROR_CODE(exit_code, "Error in IR parsing");
   return ir_managerRef();
}
