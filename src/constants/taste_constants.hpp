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
 *              Copyright (C) 2015-2020 Politecnico di Milano
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
 * @file taste_constants.hpp
 * @brief constants used in taste
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// The suffix of the address translation function
#define STR_CST_taste_address_translation "panda_address_translation_"

/// The suffix of the data size function
#define STR_CST_taste_data_size "panda_data_size_"

/// The suffix of the endianess check function
#define STR_CST_taste_endianess_check "panda_endianess_check_"

/// The suffix of the endianess inversion function
#if 0
#define STR_CST_taste_endianess_inversion "panda_endianess_inversion_"
#else
#define STR_CST_taste_endianess_inversion "bswap32"
#endif

/// The suffix of the memory enabling function
#define STR_CST_taste_memory_enabling "panda_memory_enabling_"

/// The suffix of the output multiplexer function
#define STR_CST_taste_output_multiplexer "panda_output_multiplexer_"

/// The suffix of the reg_status value computation function
#define STR_CST_taste_reg_status "panda_reg_status"
