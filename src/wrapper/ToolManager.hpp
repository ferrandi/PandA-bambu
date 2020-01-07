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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file ToolManager.hpp
 * @brief Class to manage a wrapped tool
 *
 * A object used to manage the access to a wrapped tool
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _TOOL_MANAGER_HPP_
#define _TOOL_MANAGER_HPP_

#include "refcount.hpp"
CONSTREF_FORWARD_DECL(Parameter);

#include <string>
#include <vector>

/**
 * @class IcarusWrapper
 * Main class for wrapping the Icarus verilog compiler.
 */
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

   /// debug level of the class
   int debug_level;

   /**
    * Execute the command and check the return code. If an error is occurred, an exception is raised with the given message
    * If the permissive flag is given, it raises simply a warning
    * @param log_file is the file where output will be saved
    */
   int execute_command(const std::string& command, const std::string& error_message, const std::string& log_file, bool permissive = false, bool throw_message = true);

   /**
    * Check if a command exist on a given host provided a configuration script
    */
   int check_command(const std::string& command, const std::string& setupscr, const std::string& host, bool permissive = false);
   /**
    * Generate the command to the executed on the remote host
    */
   std::string create_remote_command_line(const std::vector<std::string>& parameters) const;

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
    *  Simply creates the command line starting from the list of parameters.
    *  Note that the executable has to be the first parameter
    */
   std::string create_command_line(const std::vector<std::string>& parameters) const;

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

   /**
    * Destructor
    */
   virtual ~ToolManager();

   /**
    * Configuration of the tool
    */
   void configure(const std::string& tool, const std::string& setupscr, const std::string& host = "", const std::string& remote_path = "", bool force_remote = false);

   /**
    * Execute the tool
    * @param parameters list of parameters to be given to the tool executable
    * @param output_files list of expected output files to be verified
    * @param log_file is the log file
    * @return a flag that is true if the execution has been terminated with success, false otherwise.
    */
   int execute(const std::vector<std::string>& parameters, const std::vector<std::string>& input_files, const std::vector<std::string>& output_files = std::vector<std::string>(), const std::string& log_file = std::string(), bool permissive = false);

   /**
    * Determine the relative paths of the inputs files
    */
   std::vector<std::string> determine_paths(std::vector<std::string>& files, bool overwrite = true);

   std::string determine_paths(std::string& files, bool overwrite = true);
};

/// Refcount definition for the class
typedef refcount<ToolManager> ToolManagerRef;

#endif
