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
 *              Copyright (C) 2004-2026 Politecnico di Milano
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
 * @file dbgPrintHelper.hpp
 * @brief File containing functions and utilities to support the printing of debug messagges
 *
 * File containing functions and utilities to support the printing of debug messagges
 *
 * @author Luca Fossati <fossati@elet.polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef DBGPRINTHELPER_HPP
#define DBGPRINTHELPER_HPP

#define NOUTPUT 0

#include <iostream>
#include <string>

/// no output print is performed.
#define OUTPUT_LEVEL_NONE 0
/// minimum debugging print is performed.
#define OUTPUT_LEVEL_MINIMUM 1
/// verbose debugging print is performed.
#define OUTPUT_LEVEL_VERBOSE 2
/// verbose debugging print is performed.
#define OUTPUT_LEVEL_PEDANTIC 3
/// verbose debugging print is performed.
#define OUTPUT_LEVEL_VERY_PEDANTIC 4
/// verbose debugging print is performed.
#define OUTPUT_LEVEL_VERY_VERY_PEDANTIC 5

/// no debugging print is performed.
#define DEBUG_LEVEL_NONE 0
/// minimum debugging print is performed.
#define DEBUG_LEVEL_MINIMUM 1
/// verbose debugging print is performed.
#define DEBUG_LEVEL_VERBOSE 2
/// very verbose debugging print is performed.
#define DEBUG_LEVEL_PEDANTIC 3
/// extremely verbose debugging print is performed.
#define DEBUG_LEVEL_VERY_PEDANTIC 4
/// paranoid level debugging print is performed.
#define DEBUG_LEVEL_PARANOIC 5
/// everything is printed.
#define DEBUG_LEVEL_INFINITE 11

extern size_t indentation;

extern std::ostream null_stream;

/// This is the message to be printed
extern std::string panda_message;

// If we are producing a release, then no debug message will be printed at all,
// independently of the debug level chosen: all the debug instructions are evicted
// from the code, thus speeding up the execution.
#ifdef NDEBUG

/// We are producing a release, no debug message is printed: in order to enable the production
/// of the release, just use CXXFLAGS="-DNDEBUG -DNOUTPUT"
#define PRINT_DBG_MEX(dbgLevel, curDbgLevel, mex) void(0)
#define PRINT_DBG_STRING(dbgLevel, curDbgLevel, mex) void(0)
#define INDENT_DBG_MEX(dbgLevel, curDbgLevel, mex) void(0)

#else

/// We are producing a debug version of the program, so the message is printed;
///@param dbgLevel the minimum debug level at which we desire to print the message:
/// if the actual debug level is smaller than that, no message is printed
///@param curDbgLevel the current debug level at which the algorithm is executing
///@param mex the array of chars containing the message to print: this message
/// can be written using a printf like syntax.
#define PRINT_DBG_MEX(dbgLevel, curDbgLevel, mex) \
   (((dbgLevel) <= (curDbgLevel)) ? std::cerr << mex << std::endl : std::cerr)

/**
 * We are producing a debug version of the program, so the message is printed;
 * no newline is added;
 * @param dbgLevel the minimum debug level at which we desire to print the message:
 * if the actual debug level is smaller than that, no message is printed
 * @param curDbgLevel the current debug level at which the algorithm is executing
 * @param mex the array of chars containing the message to print: this message
 * can be written using a printf like syntax.
 */
#define PRINT_DBG_STRING(dbgLevel, curDbgLevel, mex) (((dbgLevel) <= (curDbgLevel)) ? std::cerr << mex : std::cerr)

/// We are producing a debug version of the program, so the message is printed;
///@param dbgLevel the minimum debug level at which we desire to print the message:
/// if the actual debug level is smaller than that, no message is printed
///@param curDbgLevel the current debug level at which the algorithm is executing
///@param mex the array of chars containing the message to print: this message
/// can be written using a printf like syntax.
#define INDENT_DBG_MEX(dbgLevel, curDbgLevel, mex)                                                                   \
   (((dbgLevel) <= (curDbgLevel)) ?                                                                                  \
        (panda_message = mex,                                                                                        \
         (std::string(panda_message) == "-->") ?                                                                     \
             (null_stream << (indentation += 2)) :                                                                   \
             ((std::string(panda_message).substr(0, 3) == "-->") ?                                                   \
                  (std::cerr << std::string(indentation += 2, ' ') << std::string(panda_message).substr(3)           \
                             << std::endl) :                                                                         \
                  ((std::string(panda_message).substr(0, 3) == "---") ?                                              \
                       (std::cerr << std::string(indentation + 2, ' ') << std::string(panda_message).substr(3)       \
                                  << std::endl) :                                                                    \
                       ((std::string(panda_message) == "<--") ?                                                      \
                            (null_stream << (indentation -= 2)) :                                                    \
                            ((std::string(panda_message).substr(0, 3) == "<--") ?                                    \
                                 (std::cerr << std::string(indentation, ' ') << std::string(panda_message).substr(3) \
                                            << std::endl,                                                            \
                                  null_stream << (indentation -= 2)) :                                               \
                                 (std::cerr << std::string(indentation, ' ') << panda_message << std::endl)))))) :   \
        (std::cerr))

#endif

#if NOUTPUT

#define INDENT_OUT_MEX(profLevel, curprofLevel, mex) (void(0))
#define PRINT_OUT_MEX(profLevel, curprofLevel, mex) (void(0))
#define PRINT_OUT_STRING(profLevel, curprofLevel, mex) (void(0))

#else

#define INDENT_OUT_MEX(outLevel, curOutLevel, mex)                                                                   \
   (((outLevel) <= (curOutLevel)) ?                                                                                  \
        (panda_message = mex,                                                                                        \
         (std::string(panda_message) == "-->") ?                                                                     \
             (null_stream << (indentation += 2)) :                                                                   \
             ((std::string(panda_message).substr(0, 3) == "-->") ?                                                   \
                  (std::cerr << std::string(indentation += 2, ' ') << std::string(panda_message).substr(3)           \
                             << std::endl) :                                                                         \
                  ((std::string(panda_message).substr(0, 3) == "---") ?                                              \
                       (std::cerr << std::string(indentation + 2, ' ') << std::string(panda_message).substr(3)       \
                                  << std::endl) :                                                                    \
                       ((std::string(panda_message) == "<--") ?                                                      \
                            (null_stream << (indentation -= 2)) :                                                    \
                            ((std::string(panda_message).substr(0, 3) == "<--") ?                                    \
                                 (std::cerr << std::string(indentation, ' ') << std::string(panda_message).substr(3) \
                                            << std::endl,                                                            \
                                  null_stream << (indentation -= 2)) :                                               \
                                 (std::cerr << std::string(indentation, ' ') << panda_message << std::endl)))))) :   \
        (std::cerr))

#define PRINT_OUT_MEX(profLevel, curprofLevel, mex) \
   (((profLevel) <= (curprofLevel)) ? std::cerr << mex << std::endl : std::cerr)

#define PRINT_OUT_STRING(profLevel, curprofLevel, mex) (((profLevel) <= (curprofLevel)) ? std::cerr << mex : std::cerr)

#endif

#define PRINT_MSG(mex) std::cerr << mex << std::endl
#define PRINT_STRING(mex) std::cerr << mex

#endif
