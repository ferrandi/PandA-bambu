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
 *              Copyright (c) 2004-2018 Politecnico di Milano
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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 */

#include "refcount.hpp"

#include "UnfoldedCallGraph.hpp"

#include <map>
#include <stack>
#include <string>

REF_FORWARD_DECL(HLS_manager);
CONSTREF_FORWARD_DECL(Parameter);

class HWCallPathCalculator : public boost::default_dfs_visitor
{
 protected:
   /// a refcount to the HLS_manager
   const HLS_managerRef HLSMgr;

   /// a stack of scopes used during the traversal of the UnfoldedCallGraph
   std::stack<std::string> scope;

   /// The key is the name of a shared function, the mapped value is the HW
   /// scope of that shared function
   std::map<std::string, std::string> shared_fun_scope;

   /// The scope of the top function. It depends on different parameters and
   ///  it is computed by start_vertex
   std::string top_fun_scope;

 public:
   HWCallPathCalculator(const HLS_managerRef _HLSMgr);
   ~HWCallPathCalculator();

   void start_vertex(const UnfoldedVertexDescriptor& v, const UnfoldedCallGraph& ucg);
   void discover_vertex(const UnfoldedVertexDescriptor& v, const UnfoldedCallGraph& ucg);
   void finish_vertex(const UnfoldedVertexDescriptor& v, const UnfoldedCallGraph&);
   void examine_edge(const EdgeDescriptor& e, const UnfoldedCallGraph& cg);
};
