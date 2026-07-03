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
 * @file ToolManager.hpp
 * @brief Class to manage a wrapped tool
 *
 * A object used to manage the access to a wrapped tool
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef _TOOL_MANAGER_HPP_
#define _TOOL_MANAGER_HPP_

#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);

#include <filesystem>
#include <string>
#include <vector>

class ToolManager
{
 protected:
   /// The set of parameters passed to the tool
   const ParameterConstRef Param;

   /// this string represent the path of the executable
   std::string executable;

   /// this string has the script used to setup the environment for the executable
   std::string setup_script;

   /// flag to specify if the executable is local (true) or remote (false)
   bool local;

   /// this string represent the host machine if remote
   std::string host;

   /// it represents the paths on the host where the files have to be copied
   std::string remote_path;

   std::filesystem::path outerr;

   /// debug level of the class
   int debug_level;

   /**
    * Execute the command and check the return code. If an error is occurred, an exception is raised with the given
    * message If the permissive flag is given, it raises simply a warning
    * @param _command_ is the command to execute
    * @param error_message is the message reported on failure
    * @param log_file is the file where output will be saved
    * @param permissive controls whether failures are downgraded to warnings
    * @param throw_message controls whether the error message is thrown
    */
   int execute_command(const std::string& _command_, const std::string& error_message, const std::string& log_file,
                       bool permissive = false, bool throw_message = true);

   /**
    * Check if a command exist on a given host provided a configuration script
    */
   int check_command(const std::string& _tool_, const std::string& setupscr, const std::string& _host_,
                     bool permissive = false);
   /**
    * Generate the command to the executed on the remote host
    */
   std::string create_remote_command_line(const std::string& remote_command) const;

   /**
    * Check that the input files exist.
    * If the execution is remote, it also copies the files to the remote path
    */
   void prepare_input_files(const std::vector<std::string>& files);

   /**
    * Check that the output files have been correctly generated.
    * If the execution is remote, it also copies the files in local
    */
   void check_output_files(const std::vector<std::string>& files);

   /**
    *  Removed the specified files.
    */
   void remove_files(const std::vector<std::string>& input_files, const std::vector<std::string>& files);

 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    */
   explicit ToolManager(const ParameterConstRef& Param);

   virtual ~ToolManager();

   /**
    * Configuration of the tool
    */
   void configure(const std::string& _tool_, const std::string& setupscr, const std::string& _host_ = "",
                  const std::string& _remote_path_ = "", bool force_remote = false);

   /**
    * Execute the tool
    * @param parameters list of parameters to be given to the tool executable
    * @param input_files list of input files to be provided to the tool
    * @param output_files list of expected output files to be verified
    * @param log_file is the log file
    * @param permissive controls whether failures are downgraded to warnings
    * @return the return value of the executed process.
    */
   int execute(const std::vector<std::string>& parameters, const std::vector<std::string>& input_files,
               const std::vector<std::string>& output_files = std::vector<std::string>(),
               const std::string& log_file = std::string(), bool permissive = false);

   /**
    * Determine the relative paths of the inputs files
    */
   std::vector<std::string> determine_paths(std::vector<std::string>& files, bool overwrite = true);

   std::string determine_paths(std::string& file_name, bool overwrite = true);
};

#endif
