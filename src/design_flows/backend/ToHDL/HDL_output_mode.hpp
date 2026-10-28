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
 *   Copyright (C) 2024-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file HDL_output_mode.hpp
 * @brief
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _HDL_OUTPUT_MODE_H
#define _HDL_OUTPUT_MODE_H

enum HDL_output_mode
{
   HDL_OUT_MIX = 0,       // Generate a single HDL file with all hardware components from all libraries
   HDL_OUT_WORK_SEPARATE, // Generate a HDL file with all hardware components from the WORK library and a HDL file with
                          // all hardware components from non-WORK libraries
   HDL_OUT_WORK_LIBRARY,  // Generate a HDL file with all hardware components from the WORK library and a HDL file for
                          // each hardware component from non-WORK libraries
};

#endif // _HDL_OUTPUT_MODE_H