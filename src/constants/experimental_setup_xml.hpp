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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file experimental_setup_xml.hpp
 * @brief xml nodes used in file specifying experimental setup
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef EXPERIMENTAL_SETUP_XML
#define EXPERIMENTAL_SETUP_XML

/// The root node
#define STR_XML_experimental_setup_root "experimental_setup"

/// The node containing the bambu argments
#define STR_XML_experimental_setup_bambu_arguments "bambu_arguments"

/// The node containing the list of benchmarks
#define STR_XML_experimental_setup_benchmarks "benchmarks"

/// The node containing the timestamp of the data generation
#define STR_XML_experimental_setup_timestamp "timestamp"

/// The attribute containing the actual value of a node
#define STR_XML_experimental_setup_value "value"

/// The node containing the version of bambu
#define STR_XML_experimental_setup_bambu_version "bambu_version"
#endif
