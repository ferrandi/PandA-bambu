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

/// Shell function that copies the output of a command both to a log file and,
/// live, to the console. The generated scripts use it in place of the system
/// tee: tee is not part of the tool flow, and a tee dying mid-stream closes
/// the pipe and kills the command whose output was being logged. The copy is
/// performed by the record_output tool, installed next to bambu, so the run
/// depends on the tools of the distribution only: neither the system tee nor
/// python3 are needed.
constexpr auto SHELL_CST_record_output =
    R"sh(record_output() {
  local bambu_record_output="${BAMBU_HLS}/bin/record_output"
  if [ ! -x "${bambu_record_output}" ]; then
    echo "record_output: the Bambu log copier is missing at ${bambu_record_output}: check BAMBU_HLS" 1>&2
    return 127
  fi
  "${bambu_record_output}" "$@"
}
)sh";

#endif
