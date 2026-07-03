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
 *              Copyright (C) 2016-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
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
