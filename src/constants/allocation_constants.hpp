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
 * @file allocation_constants.hpp
 * @brief constants used by HLS constants
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
/// The default value for coefficient of PIPELINE STAGE 0
#define NUM_CST_allocation_default_allocation_coefficient 1.0

/// The default value for connection offset
#define NUM_CST_allocation_default_connection_offset 0.0

/// The default value used in computation of fanout delay
#define NUM_CST_allocation_default_fanout_coefficent 0.007

/// The default value used in computation of fanout delay
#define NUM_CST_allocation_default_max_fanout_size 200

/// The default number of inputs of a LUT
#define NUM_CST_allocation_default_max_lut_size 6

/// The default value used in computation of controller delay
#define NUM_CST_allocation_default_states_number_normalization 45

/// The default value used in computation of controller delay when basic block are considered
#define NUM_CST_allocation_default_states_number_normalization_BB 30

/// The default value for the connection ratio between the output delay of a carry and the setup delay
#define NUM_CST_allocation_default_output_carry_connection_ratio 0.6

/// The default value for the connection ratio between the output delay of a DSP and the setup delay
#define NUM_CST_allocation_default_output_DSP_connection_ratio 0.6
