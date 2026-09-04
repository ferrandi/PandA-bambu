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
 * @file parse_technology.hpp
 * @brief Input function used to read the technology data structures.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef TECHNOLOGYIO_HPP
#define TECHNOLOGYIO_HPP

#include "refcount.hpp"

#include <string>

REF_FORWARD_DECL(technology_manager);
CONSTREF_FORWARD_DECL(Parameter);

/**
 * Read an xml file describing the technology data structures.
 * @param f the input file name
 * @param TM is the initial technology manager.
 * @param Param is the global parameter.
 */
void read_technology_File(const std::string& f, const technology_managerRef& TM, const ParameterConstRef& Param);

#endif
