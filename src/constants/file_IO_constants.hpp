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

/// Preserve producer failures; otherwise fail if the log could not be written completely.
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
