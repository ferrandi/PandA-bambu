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
* @file plugin_topfname.cpp
* @brief Dummy Plugin. LLVM does not need this plugin but we add just for the sake of completeness.
*
* @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
*
*/
#include "plugin_includes.hpp"

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

namespace clang {


   class clang40_plugin_topfname : public PluginASTAction
   {
         std::string topfname;
      protected:
         std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &,
                                                        llvm::StringRef ) override
         {
            return llvm::make_unique<dummyConsumer>();
         }

         bool ParseArgs(const CompilerInstance &CI,
                        const std::vector<std::string> &args) override
         {
            DiagnosticsEngine &D = CI.getDiagnostics();
            for (size_t i = 0, e = args.size(); i != e; ++i)
            {
               if (args.at(i) == "-topfname")
               {
                  if (i + 1 >= e) {
                     D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                                "missing topfname argument"));
                     return false;
                  }
                  ++i;
                  topfname = args.at(i);
               }
            }
            if (!args.empty() && args.at(0) == "-help")
               PrintHelp(llvm::errs());

            if(topfname=="")
               D.Report(D.getCustomDiagID(DiagnosticsEngine::Error,
                                          "topfname not specified"));
            return true;
         }
         void PrintHelp(llvm::raw_ostream& ros)
         {
            ros << "Help for clang40_plugin_topfname plugin\n";
            ros << "-topfname <topfunctionname>\n";
            ros << "  name of the top function\n";
         }

         PluginASTAction::ActionType getActionType() override
         {
            return AddAfterMainAction;
         }
   };

}

static clang::FrontendPluginRegistry::Add<clang::clang40_plugin_topfname>
X("clang40_plugin_topfname", "Dumy plugin");


