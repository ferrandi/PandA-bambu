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
 *              Copyright (c) 2015-2017 Politecnico di Milano
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

CONSTREF_FORWARD_DECL(CallGraphManager);

class CallGraphUnfolder
{
   protected:

      /**
       * A reference to the call graph to be unfolded
       */
      const CallGraphConstRef cg;

      /**
       * The function id of the function where to start to unfold
       */
      const unsigned int root_fun_id;
      
      /// Maps every function to the calls it performs in cg.
      // Must be correctly initialized with data from cg before calling Unfold()
      const std::unordered_map<unsigned int, std::unordered_set<unsigned int> > & caller_to_call_id;

      /// Maps every id of a call site to the id of the called function
      // Must be correctly initialized with data from cg before calling Unfold()
      const std::unordered_map<unsigned int, std::unordered_set<unsigned int> > & call_to_called_id;

      /// Set of indirect calls
      // Must be correctly initialized with data from cg before calling Unfold()
      const std::unordered_set<unsigned int> & indirect_calls;

      void RecursivelyUnfold(const UnfoldedVertexDescriptor caller_v, UnfoldedCallGraph & ucg) const;

   public:

      /**
       * Unfolds the call graph cg, starting from the function with id
       * root_fun_id.
       * Returns the UnfoldedVertexDescritor representing the root of the
       * unfolded call graph
       */
      UnfoldedVertexDescriptor Unfold(UnfoldedCallGraph & ucg) const;

      CallGraphUnfolder(CallGraphManagerConstRef cgman,
            std::unordered_map<unsigned int, std::unordered_set<unsigned int> > & _caller_to_call_id,
            std::unordered_map<unsigned int, std::unordered_set<unsigned int> > & _call_to_called_id,
            std::unordered_set<unsigned int> & _indirect_calls);

      ~CallGraphUnfolder();
};
#endif
