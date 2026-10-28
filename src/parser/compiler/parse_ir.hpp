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
 * @file parse_ir.hpp
 *
 * Declaration of the function that parses the IR from a file.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef PARSE_IR_HPP
#define PARSE_IR_HPP

#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(ir_manager);

/**
 * Function that parse the dump of front-end plugin.
 *
 * @param Param is the set of input parameters
 * @param f the input file name
 * @return the IR manager associated to the parsed file.

*/
ir_managerRef ParseIRFile(const ParameterConstRef& Param, const std::string& f);

#endif
