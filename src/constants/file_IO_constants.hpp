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
 * @file file_IO_constants.hpp
 * @brief constants used in Input/Output
 *
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 *
 */

#ifndef FILE_IO_CONSTANTS_HPP
#define FILE_IO_CONSTANTS_HPP

/// The file used to store shell output
#define STR_CST_file_IO_shell_output_file "shell_output"

/// The file used to store the script shell
#define STR_CST_file_IO_shell_script "shell_script"

/// The source code default output file
#define STR_CST_file_IO_source_code_output_file "output.c"

/// Shell function that copies the output of a command both to a log file and to
/// the console. The generated scripts use it in place of the system tee: tee is
/// not part of the tool flow, and a tee dying mid-stream closes the pipe and
/// kills the command whose output was being logged.
constexpr auto SHELL_CST_record_output =
    R"sh(record_output() {
  python3 -u -c '
import sys

with open(sys.argv[1], "wb") as log_file:
    while True:
        data = sys.stdin.buffer.read1(65536)
        if not data:
            break
        log_file.write(data)
        log_file.flush()
        sys.stdout.buffer.write(data)
        sys.stdout.buffer.flush()
' "$1"
}
)sh";

#endif
