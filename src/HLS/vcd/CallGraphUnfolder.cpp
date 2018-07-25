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
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
*/

// includes from behavior/
#include "call_graph_manager.hpp"
#include "call_graph.hpp"
#include <boost/graph/graphviz.hpp>

// includes from HLS/vcd/
#include "CallGraphUnfolder.hpp"
#include "string_manipulation.hpp"          // for STR

CallGraphUnfolder::CallGraphUnfolder(CallGraphManagerConstRef cgman,
      std::unordered_map<unsigned int, std::unordered_set<unsigned int> > & _caller_to_call_id,
      std::unordered_map<unsigned int, std::unordered_set<unsigned int> > & _call_to_called_id,
      std::unordered_set<unsigned int> & _indirect_calls)
   : cg(cgman->CGetCallGraph()),
   root_fun_id(*(cgman->GetRootFunctions().begin())),
   caller_to_call_id(_caller_to_call_id),
   call_to_called_id(_call_to_called_id),
   indirect_calls(_indirect_calls)
{
   THROW_ASSERT(cgman->GetRootFunctions().size() == 1, STR(cgman->GetRootFunctions().size()));
}

CallGraphUnfolder::~CallGraphUnfolder()
{}

UnfoldedVertexDescriptor
CallGraphUnfolder::Unfold(UnfoldedCallGraph & ucg) const
{
   // insert in the unfolded call graph the root function node
   UnfoldedVertexDescriptor v = ucg.AddVertex(NodeInfoRef(new UnfoldedFunctionInfo(root_fun_id)));
   const auto b = cg->CGetCallGraphInfo()->behaviors.find(root_fun_id);
   THROW_ASSERT(b != cg->CGetCallGraphInfo()->behaviors.end(), "no behavior for root function " + STR(root_fun_id));
   get_node_info<UnfoldedFunctionInfo>(v, ucg)->behavior = b->second;
   RecursivelyUnfold(v, ucg);
   return v;
}

void CallGraphUnfolder::RecursivelyUnfold(const UnfoldedVertexDescriptor caller_v,
   UnfoldedCallGraph & ucg) const
{
   const unsigned int caller_id = get_node_info<UnfoldedFunctionInfo>(caller_v, ucg)->f_id;
   // if this function does not call other functions we're done
   std::unordered_map<unsigned int, std::unordered_set<unsigned int> >::const_iterator caller =
      caller_to_call_id.find(caller_id);
   if (caller == caller_to_call_id.cend())
      return;

   for (const unsigned int call_id : caller->second) // loop on the calls performed by function caller_id
   {
      if (call_id == 0) // this should happen only for artificial calls
         continue;
      bool is_direct = indirect_calls.find(call_id) == indirect_calls.end();
      for (auto called_id : call_to_called_id.at(call_id)) // loop on the function called by call_id
      {
         // add a new copy of the vertex representing the called function
         UnfoldedVertexDescriptor called_v = ucg.AddVertex(NodeInfoRef(new UnfoldedFunctionInfo(called_id)));
         // update the behavior of the new vertex
         const std::map<unsigned int, FunctionBehaviorRef> & behaviors = cg->CGetCallGraphInfo()->behaviors;
         const auto b = behaviors.find(called_id);
         if (b != behaviors.end()) // the behavior can be not present if the called function is without body
            get_node_info<UnfoldedFunctionInfo>(called_v, ucg)->behavior = b->second;
         // add an edge between the caller and the called
         ucg.AddEdge(caller_v, called_v, EdgeInfoRef(new UnfoldedCallInfo(call_id, is_direct)));
         RecursivelyUnfold(called_v, ucg);
      }
   }
}
