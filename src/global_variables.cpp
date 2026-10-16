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
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file global_variables.cpp
 * @brief global variables used by each tool
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

/// NOTE: this file must be included only by source code of the executable (i.e., the file with the main)

/// STD include
#include <cstdlib>
#include <iostream>
#include <string>

/// Exit code
int exit_code = EXIT_FAILURE;

/// The current indentation for debug messages
size_t indentation = 0;

/// Mull stream
std::ostream null_stream(nullptr);

/// The current message to be printed
std::string panda_message;

/// Transform warning into errors
bool error_on_warning = false;
