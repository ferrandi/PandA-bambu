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
  local bambu_record_output
  # bin/ is the installed layout, src/ is where the CMake build tree puts the tool.
  for bambu_record_output in "${BAMBU_HLS}/bin/record_output" "${BAMBU_HLS}/src/record_output"; do
    if [ -x "${bambu_record_output}" ]; then
      "${bambu_record_output}" "$@"
      return $?
    fi
  done
  echo "record_output: the Bambu log copier is missing in ${BAMBU_HLS}/bin and ${BAMBU_HLS}/src: check BAMBU_HLS" 1>&2
  return 127
}
)sh";

/// Tail that the generated command scripts append after a command whose
/// output was piped through record_output: it saves both pipeline statuses
/// and it never hides the copier failure. The producer status wins when it
/// is not zero, otherwise the copier status is returned, so a log that
/// could not be completely written is never judged as a success, exactly
/// like run_logged does in the simulation and backend scripts. The copier
/// keeps draining the input, therefore its failure cannot be observed
/// through a SIGPIPE on the producer anymore and it must be checked.
constexpr auto SHELL_CST_record_output_status =
    R"sh(pipeline_status=("${PIPESTATUS[@]}")
if [ "${pipeline_status[1]}" -ne 0 ]; then
  echo "ERROR: log writer failed (${pipeline_status[1]}): the log may be incomplete" 1>&2
fi
if [ "${pipeline_status[0]}" -ne 0 ]; then
  exit "${pipeline_status[0]}"
fi
exit "${pipeline_status[1]}"
)sh";

#endif
