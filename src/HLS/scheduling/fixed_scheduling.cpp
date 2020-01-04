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
 * @file fixed_scheduling.cpp
 * @brief
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by
 *
 */
#include "fixed_scheduling.hpp"

#include "hls.hpp"
#include "hls_manager.hpp"

#include "Parameter.hpp"
#include "scheduling.hpp"

#include "fu_binding.hpp"
#include "schedule.hpp"

#include "exceptions.hpp"
#include "fileIO.hpp"

#include "xml_document.hpp"
#include "xml_dom_parser.hpp"

#include <iosfwd>
#include <string>

#include "dbgPrintHelper.hpp"

fixed_scheduling::fixed_scheduling(const ParameterConstRef _parameters, const HLS_managerRef _HLSMgr, unsigned int _funId, const DesignFlowManagerConstRef _design_flow_manager)
    : Scheduling(_parameters, _HLSMgr, _funId, _design_flow_manager, HLSFlowStep_Type::FIXED_SCHEDULING)
{
}

fixed_scheduling::~fixed_scheduling() = default;

DesignFlowStep_Status fixed_scheduling::InternalExec()
{
   std::string File = parameters->getOption<std::string>("fixed_scheduling_file");

   PRINT_DBG_MEX(DEBUG_LEVEL_VERBOSE, debug_level, "Reading file: " + File);
   try
   {
      XMLDomParser parser(File);
      parser.Exec();
      if(parser)
      {
         // Walk the tree:
         const xml_element* node = parser.get_document()->get_root_node(); // deleted by DomParser.
         HLS->xload(node, HLSMgr->CGetFunctionBehavior(funId)->CGetOpGraph(FunctionBehavior::FDFG));
      }
   }
   catch(const char* msg)
   {
      std::cerr << msg << std::endl;
      THROW_ERROR("Error in fixed_scheduling");
   }
   catch(const std::string& msg)
   {
      std::cerr << msg << std::endl;
      THROW_ERROR("Error in fixed_scheduling");
   }
   catch(const std::exception& ex)
   {
      std::cout << "Exception caught: " << ex.what() << std::endl;
      THROW_ERROR("Error in fixed_scheduling");
   }
   catch(...)
   {
      std::cerr << "unknown exception" << std::endl;
      THROW_ERROR("Error in fixed_scheduling");
   }
   return DesignFlowStep_Status::SUCCESS;
}
