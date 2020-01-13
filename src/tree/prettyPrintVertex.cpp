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
 * @file prettyPrintVertex.cpp
 * @brief Helper class supporting the printing of vertexes of a graph.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Lattuada <lattuada@elet.polimi.it>
 * @version $Revision$
 * @date $Date$
 * Last modified by $Author$
 *

*/
/// Header include
#include "prettyPrintVertex.hpp"

/// Behavior include
#include "behavioral_helper.hpp"
#include "op_graph.hpp"

/// utility include
#include "simple_indent.hpp" // for simple_indent
#include "var_pp_functor.hpp"

void prettyPrintVertex::get_internal_vars(const vertex& v, const OpGraphConstRef g, CustomUnorderedSet<unsigned int>& list_of_variables, const BehavioralHelperConstRef BH)
{
   const unsigned int node_id = g->CGetOpNodeInfo(v)->GetNodeId();
   if(node_id)
   {
      unsigned intermediate_var = BH->get_intermediate_var(node_id);
      if(intermediate_var && !BH->is_an_indirect_ref(intermediate_var) && !BH->is_a_constant(intermediate_var) && !BH->is_a_component_ref(intermediate_var) && !BH->is_an_array_ref(intermediate_var) && !BH->is_an_addr_expr(intermediate_var))
      {
         list_of_variables.insert(intermediate_var);
      }
      else if(intermediate_var && BH->is_an_addr_expr(intermediate_var))
      {
         unsigned int var = BH->get_operand_from_unary_expr(intermediate_var);
         while(true)
         {
            if(BH->is_a_component_ref(var))
            {
               var = BH->get_component_ref_record(var);
               continue;
            }
            if(BH->is_an_array_ref(var))
            {
               var = BH->get_array_ref_array(var);
               continue;
            }
            if(BH->is_an_indirect_ref(var))
            {
               var = BH->get_indirect_ref_var(var);
               continue;
            }
            break;
         }
         if(!BH->is_a_constant(var))
            list_of_variables.insert(var);
      }
   }
}

void prettyPrintVertex::print_forward_declaration(std::ostream& os, unsigned int type, simple_indent& PP, const BehavioralHelperConstRef BH)
{
   PP(os, BH->print_forward_declaration(type) + ";\n");
}
