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
 * @file structuralIO.hpp
 * @brief Input and output functions used to read and write the structural data structures.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * @warning This file is still in a work in progress state
 * @warning Last modified by $Author$
 *
 */
#ifndef STRUCTURALIO_HPP
#define STRUCTURALIO_HPP

#include "refcount.hpp"
#include <string>

/**
 * @name forward declarations
 */
//@{
/// RefCount type definition of the structural_manager class structure
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(Parameter);
//@}

/**
 * Read an xml file describing the structural data structures.
 * @param f the input file name
 * @return the structural manager.
 */
structural_managerRef read_structural_File(const std::string& f, const ParameterRef& Param);

/**
 * Write an xml file describing the structural data structures.
 * @param f the output file name
 * @param CM is the structural manager.
 */
void write_structural_File(const std::string& f, structural_managerRef const& CM);

#endif
