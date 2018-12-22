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
 *              Copyright (c) 2015-2018 Politecnico di Milano
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
 *
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */
#ifndef CALL_GRAPH_UNFOLDER_HPP
#define CALL_GRAPH_UNFOLDER_HPP

#include "UnfoldedCallGraph.hpp"

#include <unordered_map>
#include <unordered_set>

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(HLS_manager);

class CallGraphUnfolder
{
 public:
   /**
    * Unfolds the call graph contained in HLSMgr, starting from the root function that must be unique.
    * @param [in] HLSMgr: a refcount to the HLS_manager whose call graph must be unfolded
    * @param [in]: a refcount to parameters
    * @param [out] caller_to_call_id: maps the id of a caller function to all the ids of the calls it performs
    * @param [out] call_to_called_id: maps the id of a call site onto the ids of all the
    * functions it calls. one single call site can call multiple functions if
    * function pointers are involved
    */
   static void Unfold(const HLS_managerRef& HLSMgr, const ParameterConstRef& params, std::unordered_map<unsigned int, std::unordered_set<unsigned int>>& caller_to_call_id,
                      std::unordered_map<unsigned int, std::unordered_set<unsigned int>>& call_to_called_id);
};
#endif
