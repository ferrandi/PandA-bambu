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
*              Copyright (c) 2004-2017 Politecnico di Milano
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
* @file plugin_dumpGimpleSSA.cpp
* @brief Plugin to dump functions and global variables in gimple raw format starting from LLVM IR
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*
*/
#include "plugin_includes.hpp"
#include "llvm/Support/FileSystem.h"

namespace clang
{
   void DumpGimpleRaw::DumpVersion(llvm::raw_fd_ostream &stream)
   {
      const char * panda_plugin_version = (const char *) PANDA_PLUGIN_VERSION;
      int version = __GNUC__, minor = __GNUC_MINOR__, patchlevel = __GNUC_PATCHLEVEL__;
      stream << "GCC_VERSION: \""<< version << "."<< minor << "." << patchlevel << "\"\n";
      stream << "PLUGIN_VERSION: \""<< panda_plugin_version << "\"\n";
   }

   static std::string create_file_name_string(const std::string &outdir_name, const std::string & original_filename)
   {
      std::size_t found = original_filename.find_last_of("/\\");
      std::string dump_base_name;
      if(found == std::string::npos)
         dump_base_name = original_filename;
      else
         dump_base_name = original_filename.substr(found+1);
      return outdir_name + "/" + dump_base_name + ".gimplePSSA";
   }

   DumpGimpleRaw::DumpGimpleRaw(CompilerInstance &Instance,
                          const std::string& _outdir_name, const std::string& _InFile, bool _onlyGlobals)
      : outdir_name(_outdir_name), InFile(_InFile), filename(create_file_name_string(_outdir_name,_InFile)), Instance(Instance), stream(create_file_name_string(_outdir_name,_InFile), EC, llvm::sys::fs::F_RW), onlyGlobals(_onlyGlobals)
   {
      if( EC)
      {
         DiagnosticsEngine &D = Instance.getDiagnostics();
         D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                    "not able to open the output raw file"));

      }
      DumpVersion(stream);
   }


}
