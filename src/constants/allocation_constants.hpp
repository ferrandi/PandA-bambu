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
 *   Copyright (C) 2016-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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

/// The default number of inputs of a LUT
#define NUM_CST_allocation_default_max_lut_size 6

/// The default value used in computation of controller delay
#define NUM_CST_allocation_default_states_number_normalization 1000

/// The default value used in computation of controller delay when basic block are considered
#define NUM_CST_allocation_default_states_number_normalization_BB 200

/// The default value used in computation of controller delay when basic block are considered
#define NUM_CST_allocation_default_states_number_normalization_linear_factor 20

/// The default value for the connection ratio between the output delay of a carry and the setup delay
#define NUM_CST_allocation_default_output_carry_connection_ratio 0.6

/// The default value for the connection ratio between the output delay of a DSP and the setup delay
#define NUM_CST_allocation_default_output_DSP_connection_ratio 0.6
