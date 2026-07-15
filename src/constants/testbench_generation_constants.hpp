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
 *   Copyright (C) 2018-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file testbench_generation_constants.hpp
 * @brief constants used in testbench generation
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef TESTBENCH_GENERATION_CONSTANTS_HPP
#define TESTBENCH_GENERATION_CONSTANTS_HPP

/// threshold used to switch from data directly printed to data written to a file and then copied
#define DATA_SIZE_THRESHOLD (1024 * 8)

/// Constant delay for testbench initialization. It is relevant for XILINX devices.
#define STR_CST_INIT_TIME "100"

#endif
