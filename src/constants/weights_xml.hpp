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
 * @file weights_xml.hpp
 * @brief xml nodes of weight models
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef WEIGHTS_XML_HPP
#define WEIGHTS_XML_HPP

/// The attribute containing the cycles of an operations
#define STR_XML_weights_cycles "cycles"

/// The node containing the execution time of a function
#define STR_XML_weights_function "function"

/// The node containing information about gimple based model
#define STR_XML_weights_gimple "tree_weights"

/// The node containing information about rtl mode
#define STR_XML_weights_mode "mode"

/// The attribute containing the name of the node/type/etc
#define STR_XML_weights_name "name"

/// The attribute containing the type of distribution
#define STR_XML_weights_distribution "distribution"

/// The attribute containing the processing element of the model
#define STR_XML_weights_processing_element "processing_element"

/// The root of a weights model
#define STR_XML_weights_root "model_weights"

/// The node containing information about gimple based model
#define STR_XML_weights_rtl "rtl_weights"

/// The node containing information about rtl node
#define STR_XML_weights_rtx "rtx"

/// The attribute storing the length of the sequences
#define STR_XML_weights_sequences_length "sequence_length"

/// The attribute containing the name of a symbolic stochastic variable
#define STR_XML_weights_variable "variable"

/// The attribute containing the variance of distribution probability of cycles of an operation
#define STR_XML_weights_variance "variance"

#endif
