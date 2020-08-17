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
 * @file ToolManager.cpp
 * @brief Implementation of the tool manager
 *
 * Implementation of the methods for the manager of the tool wrapped in PandA
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */

/// Includes the class definition
#include "ToolManager.hpp"

/// includes all needed Boost.Filesystem declarations
#include <boost/filesystem/convenience.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

/// Parameter includes
#include "Parameter.hpp"
#include "constant_strings.hpp"

/// Utility include
#include "fileIO.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

#define OUTPUT_FILE GetPath("__stdouterr")

// constructor
ToolManager::ToolManager(const ParameterConstRef& _Param) : Param(_Param), local(true), debug_level(_Param->get_class_debug_level(GET_CLASS(*this)))
{
}

// destructor
ToolManager::~ToolManager()
{
   if(boost::filesystem::exists(OUTPUT_FILE))
   {
      boost::filesystem::remove(OUTPUT_FILE);
   }
}

int ToolManager::execute_command(const std::string& _command_, const std::string& error_message, const std::string& log_file, bool permissive, bool throw_message)
{
   /// on Ubuntu sh is different from bash so we enforce it
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Executing command: " + _command_);
   THROW_ASSERT(!log_file.empty(), "Log file not set");
   int ret = PandaSystem(Param, _command_, log_file);
   if(IsError(ret))
   {
      if(permissive)
      {
         if(throw_message)
         {
            THROW_WARNING(error_message);
         }
      }
      else
      {
         /// Safe approach for release where assertions are disabled
         if(!log_file.empty())
         {
            CopyStdout(log_file);
         }
         THROW_ERROR(error_message);
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Executed command: " + _command_);
      return -1;
   }
   else
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Executed command: " + _command_);
      return ret;
   }
}

int ToolManager::check_command(const std::string& _tool_, const std::string& setupscr, const std::string& _host_, bool permissive)
{
   std::string command;
   if(!_host_.empty())
   {
      /// check if the command is available on host machine
      command += "ssh " + _host_ + " '";
   }
   /// add setup script execution
   if(!setupscr.empty())
   {
      if(boost::algorithm::starts_with(setupscr, "export"))
      {
         command += setupscr + " >& /dev/null; ";
      }
      else
      {
         command += ". " + setupscr + " >& /dev/null; ";
      }
   }

   command += "if test -f " + _tool_ + " ; then ";
   command += "   true; ";
   command += "else ";
   command += "   if test `which " + _tool_ + "`; then ";
   command += "      true; ";
   command += "   else ";
   command += "      false; ";
   command += "   fi ";
   command += "fi";

   if(!_host_.empty())
   {
      command += "'";
   }

   command += ">& " + std::string(OUTPUT_FILE);
   const auto ret = execute_command(command, "Problems in checking \"" + _tool_ + "\" executable" + (!_host_.empty() ? " on host \"" + _host_ + "\"" + (!setupscr.empty() ? " with this setup script \"" + setupscr + "\"!" : "") : ""),
                                    Param->getOption<std::string>(OPT_output_temporary_directory) + "/check_command_output", permissive, false);
   return ret;
}

void ToolManager::configure(const std::string& _tool_, const std::string& setupscr, const std::string& _host_, const std::string& _remote_path_, bool force_remote)
{
   setup_script = setupscr;
   /// check if the command is locally available
   executable = "";
   if(!force_remote and check_command(_tool_, setupscr, "", true) != -1)
   {
      if(!setupscr.empty())
      {
         if(boost::algorithm::starts_with(setupscr, "export"))
         {
            executable += setupscr + " >& /dev/null; ";
         }
         else
         {
            executable += ". " + setupscr + " >& /dev/null; ";
         }
      }
      executable += _tool_;
      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " Executable: " + executable);
   }
   /// check if the command is remotely available
   else if(!_host_.empty())
   {
      /// check if the host is reachable
      if(check_command(_tool_, setupscr, _host_) == -1)
      {
         THROW_ERROR("Login problems on host \"" + _host_ + "\" or executable not available!");
      }

      PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " Correctly connected to Host \"" + _host_ + "\"");
      std::string command;
      local = false;
      if(!setupscr.empty())
      {
         if(boost::algorithm::starts_with(setupscr, "export"))
         {
            executable += setupscr + "; ";
         }
         else
         {
            executable += ". " + setupscr + "; ";
         }
      }
      executable += _tool_;
      host = _host_;
      if(!_remote_path_.empty())
      {
         command = "ssh " + _host_ + " ";
         command += "'mkdir -p " + _remote_path_ + "' >& " + std::string(OUTPUT_FILE);
         execute_command(command, "Remote path cannot be created on the host machine \"" + host + "\"!", Param->getOption<std::string>(OPT_output_temporary_directory) + "/configure_output");
      }
      remote_path = _remote_path_;
   }
   else
   {
      std::ifstream output_file(OUTPUT_FILE);
      if(!force_remote && output_file.is_open() && !output_file.eof())
      {
         local = true;
         executable = _tool_;
      }
      else
      {
         THROW_ERROR("Command \"" + _tool_ + "\" not found!");
      }
   }
}

std::string ToolManager::create_command_line(const std::vector<std::string>& parameters) const
{
   THROW_ASSERT(!parameters.empty(), "Executable has not been specified");
   std::string command = parameters[0];
   for(unsigned int i = 1; i < parameters.size(); i++)
   {
      command += (" " + parameters[i]);
   }
   return command;
}

std::string ToolManager::create_remote_command_line(const std::vector<std::string>& parameters) const
{
   std::string command = create_command_line(parameters);
   return "ssh " + host + " 'cd " + remote_path + "; " + command + "'";
}

std::vector<std::string> ToolManager::determine_paths(std::vector<std::string>& files, bool overwrite)
{
   std::vector<std::string> effective_files;
   effective_files.reserve(files.size());
   for(auto& file : files)
   {
      effective_files.push_back(determine_paths(file, overwrite));
   }
   return effective_files;
}

std::string ToolManager::determine_paths(std::string& file_name, bool overwrite)
{
   std::string effective_file, file_to_be_copied;
   bool copy = false;

   boost::filesystem::path file(file_name);
   std::string FileName = GetLeafFileName(file);
   if(local)
   {
      effective_file = file_name;
   }
   else
   {
      effective_file = FileName;
   }
   if(!local and !overwrite)
   {
      std::string command = "ssh " + host + " ";
      command += "'if test -f " + remote_path + "/" + FileName + " ; then ";
      command += "   true; ";
      command += "else ";
      command += "   false; ";
      command += "fi'";
      int ret = execute_command(command, "Login problems on host \"" + host + "\"!", Param->getOption<std::string>(OPT_output_temporary_directory) + "/determine_paths_output", true);
      if(ret == -1)
      {
         copy = true;
      }
   }
   else
   {
      copy = true;
   }
   if(copy)
   {
      if(!boost::filesystem::exists(file))
      {
         THROW_ERROR("File \"" + file.string() + "\" does not exists");
      }
      file_to_be_copied = file_name;
   }

   file_name = file_to_be_copied;
   return effective_file;
}

void ToolManager::prepare_input_files(const std::vector<std::string>& files)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Preparing input files");
   std::vector<std::string> move_to_host(1, "scp");
   for(const auto& i : files)
   {
      boost::filesystem::path file(i);
      if(!boost::filesystem::exists(file))
      {
         THROW_ERROR("File \"" + file.string() + "\" does not exists");
      }
      if(!local)
      {
         move_to_host.push_back(i);
      }
   }
   if(!local and !files.empty())
   {
      move_to_host.push_back(host + ":" + remote_path);
      move_to_host.push_back(">& " + std::string(OUTPUT_FILE));
      std::string command = create_command_line(move_to_host);
      execute_command(command, "Input files cannot be moved on the host machine", Param->getOption<std::string>(OPT_output_temporary_directory) + "/prepare_input_files_output");
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Prepared input files");
}

int ToolManager::execute(const std::vector<std::string>& parameters, const std::vector<std::string>& input_files, const std::vector<std::string>& output_files, const std::string& log_file, bool permissive)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Invoking tool execution");
   THROW_ASSERT(!log_file.empty(), "Log file is empty");
   /// check that the input files exist and, if the execution is remote, copy them on the remote path
   prepare_input_files(input_files);

   /// check if the output files are already present and delete them
   remove_files(input_files, output_files);

   /// execute the command
   std::vector<std::string> command_line(1, executable);
   for(const auto& parameter : parameters)
   {
      command_line.push_back(parameter);
   }
   std::string command = local ? create_command_line(command_line) : create_remote_command_line(command_line);

   THROW_ASSERT(!log_file.empty(), "Log file not set during executable " + executable);
   execute_command(command, "Returned error code!", log_file, permissive);

   /// check that all expected files have been generated
   check_output_files(output_files);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Invoked tool execution");

   return 0;
}

void ToolManager::remove_files(const std::vector<std::string>& input_files, const std::vector<std::string>& files)
{
   std::vector<std::string> removing(1, "rm -rf");
   for(const auto& file : files)
   {
      if(boost::filesystem::exists(file) and std::find(input_files.begin(), input_files.end(), file) == input_files.end())
      {
         removing.push_back(file);
         boost::filesystem::remove(file);
      }
   }
   if(removing.size() == 1)
   {
      return;
   }
   std::string command = local ? create_command_line(removing) : create_remote_command_line(removing);
   execute_command(command, "Files cannot correctly removed", Param->getOption<std::string>(OPT_output_temporary_directory) + "/remove_files_output");
}

void ToolManager::check_output_files(const std::vector<std::string>& files)
{
   std::vector<std::string> move_from_host(1, "scp ");
   for(const auto& i : files)
   {
      if(local)
      {
         boost::filesystem::path file(i);
         if(!boost::filesystem::exists(file))
         {
            THROW_ERROR("File \"" + file.string() + "\" has not been correctly created");
         }
      }
      else
      {
         move_from_host.push_back(host + ":" + remote_path + "/" + i);
      }
   }
   if(!local)
   {
      move_from_host.emplace_back(".");
      move_from_host.push_back(">& " + std::string(OUTPUT_FILE));
      std::string command = create_command_line(move_from_host);
      auto output_level = Param->getOption<unsigned int>(OPT_output_level);
      PRINT_OUT_MEX(OUTPUT_LEVEL_VERBOSE, output_level, " Moving output files from the host machine...");
      execute_command(command, "Generated files cannot be moved from the host machine", Param->getOption<std::string>(OPT_output_temporary_directory) + "/check_output_files_output");
   }
}
