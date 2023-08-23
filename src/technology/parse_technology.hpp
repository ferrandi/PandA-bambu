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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file parse_technology.hpp
 * @brief Input and output functions used to read and write the technology data structures.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * @warning This file is still in a work in progress state
 * @warning Last modified by $Author$
 *
 */
#ifndef TECHNOLOGYIO_HPP
#define TECHNOLOGYIO_HPP

#include "refcount.hpp"
/**
 * @name forward declarations
 */
//@{
/// RefCount type definition of the technology_manager class structure
CONSTREF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_manager);
CONSTREF_FORWARD_DECL(Parameter);
class library_manager;
//@}

#include <string>

/**
 * Read an xml file describing the technology data structures.
 * @param f the input file name
 * @param TM is the initial technology manager.
 * @return the technology manager.
 */
void read_technology_File(const std::string& f, const technology_managerRef& TM, const ParameterConstRef& Param);

/**
 * Read and update a library or a set of libraries based on the parameter options
 * @param TM is the technology manager.
 * @param Param is the class containing all the parameters
 */
void read_technology_library(const technology_managerRef& TM, const ParameterConstRef& param);

/**
 * Write an xml file describing a single library.
 * @param f the output file name
 * @param LM is the pointer to the library to be written
 */
void write_xml_technology_File(const std::string& f, library_manager* LM);

#endif
