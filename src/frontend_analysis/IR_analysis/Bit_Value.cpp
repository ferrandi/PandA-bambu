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
 * @file Bit_Value.cpp
 * @brief Full implementation of Bit Value analysis as described in
 * BitValue Inference: Detecting and Exploiting Narrow Bitwidth Computations
 * Mihai Budiu Seth Copen Goldstein
 * http://www.cs.cmu.edu/~seth/papers/budiu-tr00.pdf
 * This technical report is an extension of the following paper:
 * Mihai Budiu, Majd Sakr, Kip Walker, Seth Copen Goldstein: BitValue Inference: Detecting and Exploiting Narrow Bitwidth Computations. Euro-Par 2000: 969-979
 *
 * Created on: May 27, 2014
 *
 * @author Giulio Stramondo
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Pietro Fezzardi <pietro.fezzardi@gmail.com>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */

/// Header include
#include "Bit_Value.hpp"

///. include
#include "Parameter.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// frontend_analysis
#include "application_frontend_flow_step.hpp"

/// HLS include
#include "hls_manager.hpp"
#include "hls_target.hpp"

/// HLS/memory include
#include "memory.hpp"

/// STD include
#include <cmath>
#include <fstream>
#include <string>

/// STL includes
#include <deque>
#include <tuple>
#include <utility>
#include <vector>

#include "custom_map.hpp"
#include "custom_set.hpp"

/// tree include
#include "behavioral_helper.hpp"
#include "tree_basic_block.hpp"
#include "tree_helper.hpp"
#include "tree_manager.hpp"
#include "tree_reindex.hpp"

/// wrapper/treegcc include
#include "gcc_wrapper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

const std::map<bit_lattice, std::map<bit_lattice, std::map<bit_lattice, std::deque<bit_lattice>>>> Bit_Value::plus_expr_map = {
    // a b carry
    {
        bit_lattice::X,
        {
            {
                bit_lattice::X,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::X},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::X},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ZERO, bit_lattice::X},
                    },
                },
            },
            {
                bit_lattice::ZERO,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ONE,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::U,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
        },
    },
    {
        bit_lattice::ZERO,
        {
            {
                bit_lattice::X,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ZERO,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ONE,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::U,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
        },
    },
    {
        bit_lattice::ONE,
        {
            {
                bit_lattice::X,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ZERO,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ONE,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ONE, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ONE, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::U,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
        },
    },
    {
        bit_lattice::U,
        {
            {
                bit_lattice::X,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ZERO,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ONE,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::U,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
        },
    },
};

const std::map<bit_lattice, std::map<bit_lattice, std::map<bit_lattice, std::deque<bit_lattice>>>> Bit_Value::minus_expr_map = {
    // a b borrow
    {
        bit_lattice::X,
        {
            {
                bit_lattice::X,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::X},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::X},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ZERO, bit_lattice::X},
                    },
                },
            },
            {
                bit_lattice::ZERO,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                },
            },
            {
                bit_lattice::ONE,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::U,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
        },
    },
    {
        bit_lattice::ZERO,
        {
            {
                bit_lattice::X,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ZERO,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ONE,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ONE, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ONE, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::U,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
        },
    },
    {
        bit_lattice::ONE,
        {
            {
                bit_lattice::X,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ZERO,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ONE,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::ZERO},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::ONE},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::U,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
        },
    },
    {
        bit_lattice::U,
        {
            {
                bit_lattice::X,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ZERO,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::ZERO, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::ONE,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::ONE, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
            {
                bit_lattice::U,
                {
                    {
                        bit_lattice::ZERO,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::ONE,
                        {bit_lattice::U, bit_lattice::U},
                    },
                    {
                        bit_lattice::U,
                        {bit_lattice::U, bit_lattice::U},
                    },
                },
            },
        },
    },
};

const std::map<bit_lattice, std::map<bit_lattice, bit_lattice>> Bit_Value::bit_ior_expr_map = {
    {
        bit_lattice::ZERO,
        {
            {bit_lattice::ZERO, bit_lattice::ZERO},
            {bit_lattice::ONE, bit_lattice::ONE},
            {bit_lattice::U, bit_lattice::U},
            {bit_lattice::X, bit_lattice::X},
        },
    },
    {
        bit_lattice::ONE,
        {
            {bit_lattice::ZERO, bit_lattice::ONE},
            {bit_lattice::ONE, bit_lattice::ONE},
            {bit_lattice::U, bit_lattice::ONE},
            {bit_lattice::X, bit_lattice::ONE},
        },
    },
    {
        bit_lattice::U,
        {
            {bit_lattice::ZERO, bit_lattice::U},
            {bit_lattice::ONE, bit_lattice::ONE},
            {bit_lattice::U, bit_lattice::U},
            {bit_lattice::X, bit_lattice::X},
        },
    },
    {
        bit_lattice::X,
        {
            {bit_lattice::ZERO, bit_lattice::X},
            {bit_lattice::ONE, bit_lattice::ONE},
            {bit_lattice::U, bit_lattice::X},
            {bit_lattice::X, bit_lattice::X},
        },
    },
};

const std::map<bit_lattice, std::map<bit_lattice, bit_lattice>> Bit_Value::bit_xor_expr_map = {
    {
        bit_lattice::ZERO,
        {
            {bit_lattice::ZERO, bit_lattice::ZERO},
            {bit_lattice::ONE, bit_lattice::ONE},
            {bit_lattice::U, bit_lattice::U},
            {bit_lattice::X, bit_lattice::X},
        },
    },
    {
        bit_lattice::ONE,
        {
            {bit_lattice::ZERO, bit_lattice::ONE},
            {bit_lattice::ONE, bit_lattice::ZERO},
            {bit_lattice::U, bit_lattice::U},
            {bit_lattice::X, bit_lattice::X},
        },
    },
    {
        bit_lattice::U,
        {
            {bit_lattice::ZERO, bit_lattice::U},
            {bit_lattice::ONE, bit_lattice::U},
            {bit_lattice::U, bit_lattice::U},
            {bit_lattice::X, bit_lattice::X},
        },
    },
    {
        bit_lattice::X,
        {
            {bit_lattice::ZERO, bit_lattice::X},
            {bit_lattice::ONE, bit_lattice::X},
            {bit_lattice::U, bit_lattice::X},
            {bit_lattice::X, bit_lattice::X},
        },
    },
};

const std::map<bit_lattice, std::map<bit_lattice, bit_lattice>> Bit_Value::bit_and_expr_map = {
    {
        bit_lattice::ZERO,
        {
            {bit_lattice::ZERO, bit_lattice::ZERO},
            {bit_lattice::ONE, bit_lattice::ZERO},
            {bit_lattice::U, bit_lattice::ZERO},
            {bit_lattice::X, bit_lattice::ZERO},
        },
    },
    {
        bit_lattice::ONE,
        {
            {bit_lattice::ZERO, bit_lattice::ZERO},
            {bit_lattice::ONE, bit_lattice::ONE},
            {bit_lattice::U, bit_lattice::U},
            {bit_lattice::X, bit_lattice::X},
        },
    },
    {
        bit_lattice::U,
        {
            {bit_lattice::ZERO, bit_lattice::ZERO},
            {bit_lattice::ONE, bit_lattice::U},
            {bit_lattice::U, bit_lattice::U},
            {bit_lattice::X, bit_lattice::X},
        },
    },
    {
        bit_lattice::X,
        {
            {bit_lattice::ZERO, bit_lattice::ZERO},
            {bit_lattice::ONE, bit_lattice::X},
            {bit_lattice::U, bit_lattice::X},
            {bit_lattice::X, bit_lattice::X},
        },
    },
};

unsigned int Bit_Value::pointer_resizing(unsigned int output_id) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Pointer resizing starting from " + TM->CGetTreeNode(output_id)->ToString());
   unsigned int var = tree_helper::get_base_index(TM, output_id);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Base variable is " + TM->CGetTreeNode(var)->ToString());
   unsigned int address_bitsize;
   if(not_frontend)
   {
      auto* hm = GetPointer<HLS_manager>(AppM);
      if(hm and hm->Rmem)
      {
         if(var and function_behavior->is_variable_mem(var))
         {
            unsigned long long int max_addr = hm->Rmem->get_base_address(var, function_id) + BitLatticeManipulator::size(TM, var) / 8;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Maximum address is " + STR(max_addr - 1));
            for(address_bitsize = 1; max_addr > (1ull << address_bitsize); ++address_bitsize)
            {
               ;
            }
            /// check if it clash with the alignment:
            auto vd = GetPointer<const var_decl>(TM->CGetTreeNode(var));
            if(hm->Rmem->get_base_address(var, function_id) == 0 && vd)
            {
               auto align = vd->algn;
               align = align < 8 ? 1 : (align / 8);
               auto index = 0u;
               bool found = false;
               for(; index < address_bitsize; ++index)
                  if((1ULL << index) & align)
                  {
                     found = true;
                     break;
                  }
               if(!found)
                  ++address_bitsize;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Memory variable " + STR(address_bitsize));
         }
         else
         {
            address_bitsize = hm->get_address_bitsize();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Address bus bitsize " + STR(address_bitsize));
         }
      }
      else
      {
         address_bitsize = AppM->get_address_bitsize();
         INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Address bitsize " + STR(address_bitsize));
      }
   }
   else
   {
      address_bitsize = static_cast<unsigned int>(GccWrapper::CGetPointerSize(parameters));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Pointer bitsize " + STR(address_bitsize));
   }
   return address_bitsize;
}

unsigned int Bit_Value::lsb_to_zero(const addr_expr* ae, bool safe) const
{
   auto vd = GetPointer<var_decl>(GET_NODE(ae->op));
   if(!vd)
      return 0;
   auto align = vd->algn;
   if(safe)
   {
      align = align < 8 ? 1 : (align / 8);
   }
   else
   {
      align = align < 64 ? 8 : (align / 8);
   }
   auto index = 0u;
   bool found = false;
   for(; index < AppM->get_address_bitsize(); ++index)
      if((1ULL << index) & align)
      {
         found = true;
         break;
      }
   return found ? index : 0;
}

Bit_Value::Bit_Value(const ParameterConstRef params, const application_managerRef AM, unsigned int f_id, const DesignFlowManagerConstRef dfm)
    : FunctionFrontendFlowStep(AM, f_id, BIT_VALUE, dfm, params), BitLatticeManipulator(AM->get_tree_manager(), parameters->get_class_debug_level(GET_CLASS(*this))), not_frontend(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

Bit_Value::~Bit_Value() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>> Bit_Value::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case(PRECEDENCE_RELATIONSHIP):
      {
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(DEAD_CODE_ELIMINATION, SAME_FUNCTION));
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         break;
      }
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_PARM_MASK, WHOLE_APPLICATION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(INVALIDATION_RELATIONSHIP):
      {
         break;
      }
      default:
         THROW_UNREACHABLE("");
   }
   return relationships;
}

// prints the content of a bitstring map
void Bit_Value::print_bitstring_map(const CustomUnorderedMap<unsigned int, std::deque<bit_lattice>>&
#ifndef NDEBUG
                                        map
#endif
) const
{
   const BehavioralHelperConstRef BH = AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper();
#ifndef NDEBUG
   for(const auto& m : map)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "var_uid: " + STR(m.first) + ":" + BH->PrintVariable(m.first) + " bitstring: " + bitstring_to_string(m.second));
   }
#endif
}

bool Bit_Value::update_IR()
{
   bool res = false;
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Updating IR");
   for(const auto& b : best)
   {
#ifndef NDEBUG
      if(not AppM->ApplyNewTransformation())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         return res;
      }
#endif
      const unsigned int tn_id = b.first;
      tree_nodeRef tn = TM->get_tree_node_const(tn_id);
      const auto kind = tn->get_kind();
      if(kind == ssa_name_K)
      {
         auto* ssa = GetPointer<ssa_name>(tn);
         THROW_ASSERT(ssa, "not ssa");
         if(ssa->bit_values.empty() or ssa->bit_values.size() > b.second.size())
         {
            ssa->bit_values = bitstring_to_string(b.second);
            res = true;
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "var id: " + STR(tn_id) + " bitstring: " + ssa->bit_values);
#ifndef NDEBUG
            AppM->RegisterTransformation(GetName(), tn);
#endif
         }
         if(ssa->var != nullptr and GET_NODE(ssa->var)->get_kind() == parm_decl_K)
         {
            const auto def = ssa->CGetDefStmts();
            THROW_ASSERT(not def.empty(), GET_NODE(tn)->ToString() + " is a ssa_name but has no def_stmts");
            if(def.size() == 1 and ((GET_NODE((*def.begin()))->get_kind() == gimple_nop_K) or ssa->volatile_flag))
            {
               // ssa is the first version of a parameter
               THROW_ASSERT(ssa->bit_values.empty() or ssa->bit_values.size() <= b.second.size(), "old bit values string: " + STR(ssa->bit_values.size()) + "new bit value: " + STR(b.second.size()));
               const tree_nodeRef p = GET_NODE(ssa->var);
#ifndef NDEBUG
               const auto* pd = GetPointer<const parm_decl>(p);
#endif
               /*
                * don't update the bit values of parm_decl here because it would
                * change how the bitvalue of this function is seen from outside.
                * for this reason this operation must be performed in BitValueIPA
                * pd->bit_values = bitstring_to_string(b.second);
                * res= true;
                */
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "var id: " + STR(tn_id) + " is first version of parameter: " + STR(pd->index));
            }
         }
      }
      else if(kind == function_decl_K)
      {
#if HAVE_ASSERTS
         const auto* fd = GetPointer<const function_decl>(tn);
#endif
         THROW_ASSERT(fd, "not a function_decl");
         THROW_ASSERT(fd->index == function_id, "unexpected function id");
         THROW_ASSERT(fd->bit_values.empty() or fd->bit_values.size() <= b.second.size(), "old bit values string: " + STR(fd->bit_values.size()) + "new bit value: " + STR(b.second.size()));
         /*
          * don't update the bit values of function_decl here because it would
          * change how the bitvalue of this function is seen from outside.
          * for this reason this operation must be performed in BitValueIPA
          * fd->bit_values = bitstring_to_string(b.second);
          * res= true;
          */
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "fun id: " + STR(tn_id) + " bitstring: " + fd->bit_values);
      }
      else if(kind == integer_cst_K or kind == real_cst_K)
      {
         // do nothing, constants are recomputed every time
         ;
      }
      else
      {
         THROW_ERROR("unexpected condition: variable of kind " + tn->get_kind_text());
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--Updated IR");

   return res;
}

void Bit_Value::initialize()
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Internal initialize");
   BitLatticeManipulator::clear();
   direct_call_id_to_called_id.clear();
   arguments.clear();

   const CallGraphManagerConstRef CGMan = AppM->CGetCallGraphManager();
   const CallGraphConstRef cg = CGMan->CGetCallGraph();
   const vertex v = CGMan->GetVertex(function_id);

   const auto rbf = CGMan->GetReachedBodyFunctions();
   OutEdgeIterator oe_it, oe_end;
   boost::tie(oe_it, oe_end) = boost::out_edges(v, *cg);
   for(; oe_it != oe_end; oe_it++)
   {
      const FunctionEdgeInfoConstRef call_edge_info = cg->CGetFunctionEdgeInfo(*oe_it);
      for(const auto i : call_edge_info->direct_call_points)
      {
         const unsigned int called_id = CGMan->get_function(boost::target(*oe_it, *cg));
         if(i == 0)
         {
            // never analyze artificial calls
#if HAVE_ASSERTS
            const FunctionBehaviorConstRef FB = AppM->CGetFunctionBehavior(function_id);
            THROW_ASSERT(AppM->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->get_function_name() == "__internal_bambu_memcpy",
                         "function " + FB->CGetBehavioralHelper()->get_function_name() + " calls function " + AppM->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->get_function_name() + " with an artificial call: this should not happen");
#endif
            continue;
         }
         if(rbf.find(called_id) != rbf.end())
         {
            direct_call_id_to_called_id[i] = called_id;
         }
      }
   }

   tree_nodeRef tn = TM->get_tree_node_const(function_id);
   auto* fd = GetPointer<function_decl>(tn);
   THROW_ASSERT(fd && fd->body, "Node is not a function or it hasn't a body");

   /*
    * loop on the list of arguments and extract the bitvalue strings that have
    * been initialized by the IPA, if any
    */
   CustomUnorderedMap<unsigned int, std::deque<bit_lattice>> parm;
   for(const auto& parm_decl_node : fd->list_of_args)
   {
      unsigned int p_decl_id = GET_INDEX_NODE(parm_decl_node);
      const tree_nodeConstRef parm_type = tree_helper::CGetType(GET_NODE(parm_decl_node));
      if(not is_handled_by_bitvalue(parm_type->index))
         continue;
      auto* p = GetPointer<parm_decl>(GET_NODE(parm_decl_node));
      std::deque<bit_lattice> b = p->bit_values.empty() ? create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(parm_decl_node))) : string_to_bitstring(p->bit_values);
      parm.insert(std::make_pair(p_decl_id, b));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Index node of parameter " + STR(GET_NODE(parm_decl_node)) + " inserted: " + STR(GET_INDEX_NODE(parm_decl_node)) + " bitstring: \"" + bitstring_to_string(b) + "\"");
   }

   /*
    * initialize the bitvalue strings for the return value that have been set on
    * the function_decl from the BitValueIPA
    */
   const tree_nodeConstRef fu_type = tree_helper::CGetType(tn);
   THROW_ASSERT(fu_type->get_kind() == function_type_K || fu_type->get_kind() == method_type_K, "node " + STR(function_id) + " is " + fu_type->get_kind_text());
   unsigned int ret_type_id;
   tree_nodeRef fret_type_node;
   if(fu_type->get_kind() == function_type_K)
   {
      const auto* ft = GetPointer<const function_type>(fu_type);
      ret_type_id = GET_INDEX_NODE(ft->retn);
      fret_type_node = GET_NODE(ft->retn);
   }
   else
   {
      const auto* mt = GetPointer<const method_type>(fu_type);
      ret_type_id = GET_INDEX_NODE(mt->retn);
      fret_type_node = GET_NODE(mt->retn);
   }

   if(not is_handled_by_bitvalue(ret_type_id))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "functions returning " + STR(fret_type_node) + " not considered: " + STR(ret_type_id));
   }
   else
   {
      if(fd->bit_values.empty())
         best[function_id] = create_u_bitstring(BitLatticeManipulator::Size(fret_type_node));
      else
         best[function_id] = string_to_bitstring(fd->bit_values);

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Index node of current function inserted: " + STR(function_id));
      if(tree_helper::is_int(TM, ret_type_id))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
         signed_var.insert(function_id);
      }
   }

   const auto* sl = GetPointer<const statement_list>(GET_NODE(fd->body));
   {
      /*
       * Compute initialization bitstrings for ssa loaded from ROMs. This
       * initialization has to be performed before the other ssa are initialized
       * because we must be sure that bitstrings computed in previous executions
       * of bitvalues analysis are not thrown away before computing inf() on
       * bitvalues used for ROMs. If this initialization is interleaved with the
       * initialization of bitvalues of other ssa we may lose some information,
       * because some of the old bitstrings attached to ssa are cleared during the
       * initialization. If this happens, optimizations on ROMs cannot be
       * aggressive enough, with worse cycles and DSP usage for CHStone benchmarks
       */
      std::map<unsigned int, std::deque<bit_lattice>> private_variables;
      for(const auto& B_it : sl->list_of_bloc)
      {
         blocRef B = B_it.second;
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "analyzing BB: " + STR(B->number));

         for(const auto& stmt : B->CGetStmtList())
         {
            const auto stmt_node = GET_NODE(stmt);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "stmt " + STR(stmt_node) + " kind: " + stmt_node->get_kind_text());
            if(stmt_node->get_kind() == gimple_assign_K)
            {
               const auto* ga = GetPointer<const gimple_assign>(stmt_node);
               THROW_ASSERT(not ga->clobber, "");

               // handle lhs
               auto* lhs_ssa = GetPointer<ssa_name>(GET_NODE(ga->op0));
               if(lhs_ssa)
               {
                  unsigned int ssa_node_id = lhs_ssa->index;
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "assignment index: " + STR(GET_INDEX_NODE(stmt)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "LHS index: " + STR(ssa_node_id));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "operation kind: " + GET_NODE(ga->op1)->get_kind_text());
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "bloc: " + STR(B->number));

                  if(not is_handled_by_bitvalue(ssa_node_id))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "variable " + STR(lhs_ssa) + " of type " + STR(tree_helper::CGetType(GET_NODE(ga->op0))) + " not considered id: " + STR(ssa_node_id));
                  }
                  else
                  {
                     bool lhs_ssa_is_signed = tree_helper::is_int(TM, ssa_node_id);
                     if(lhs_ssa_is_signed)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
                        signed_var.insert(ssa_node_id);
                     }
                     /// check if this assignment is a load from a constant array
                     if(GET_NODE(ga->op1)->get_kind() == array_ref_K || GET_NODE(ga->op1)->get_kind() == mem_ref_K || GET_NODE(ga->op1)->get_kind() == target_mem_ref_K || GET_NODE(ga->op1)->get_kind() == target_mem_ref461_K ||
                        GET_NODE(ga->op1)->get_kind() == var_decl_K)
                     {
                        unsigned int base_index = tree_helper::get_base_index(TM, GET_INDEX_NODE(ga->op1));
                        auto* hm = GetPointer<HLS_manager>(AppM);
                        if(base_index && AppM->get_written_objects().find(base_index) == AppM->get_written_objects().end() && hm && hm->Rmem && function_behavior->is_variable_mem(base_index) && hm->Rmem->is_sds_var(base_index))
                        {
                           tree_nodeRef var_node = TM->get_tree_node_const(base_index);
                           if(var_node->get_kind() == var_decl_K)
                           {
                              auto* vd = GetPointer<var_decl>(var_node);
                              if(vd->init)
                              {
                                 std::deque<bit_lattice> current_inf;
                                 if(GET_NODE(vd->init)->get_kind() == constructor_K)
                                 {
                                    current_inf = constructor_bitstring(GET_NODE(vd->init), ssa_node_id);
                                 }
                                 else if(GET_NODE(vd->init)->get_kind() == integer_cst_K)
                                 {
                                    auto* int_const = GetPointer<integer_cst>(GET_NODE(vd->init));
                                    current_inf = create_bitstring_from_constant(int_const->value, BitLatticeManipulator::Size(GET_NODE(vd->init)), tree_helper::is_int(TM, GET_INDEX_NODE(int_const->type)));
                                 }
                                 else if(GET_NODE(vd->init)->get_kind() == real_cst_K)
                                 {
                                    auto* real_const = GetPointer<real_cst>(GET_NODE(vd->init));
                                    const auto real_size = BitLatticeManipulator::Size(GET_CONST_NODE(real_const->type));
                                    THROW_ASSERT(real_size == 64 || real_size == 32, "Unhandled real type size (" + STR(real_size) + ")");
                                    if(real_const->valx.front() == '-' && real_const->valr.front() != real_const->valx.front())
                                    {
                                       current_inf = string_to_bitstring(convert_fp_to_string("-" + real_const->valr, real_size));
                                    }
                                    else
                                    {
                                       current_inf = string_to_bitstring(convert_fp_to_string(real_const->valr, real_size));
                                    }
                                    sign_reduce_bitstring(current_inf, false);
                                 }
                                 else if(GET_NODE(vd->init)->get_kind() == string_cst_K)
                                 {
                                    current_inf = string_cst_bitstring(GET_NODE(vd->init), ssa_node_id);
                                 }
                                 else
                                 {
                                    current_inf = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
                                 }

                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Used the init bitstring " + bitstring_to_string(current_inf));

                                 vd->bit_values = bitstring_to_string(current_inf);
                                 best[ssa_node_id] = current_inf;
                              }
                              else
                              {
                                 best[ssa_node_id] = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
                              }
                           }
                           else
                           {
                              best[ssa_node_id] = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
                           }
                        }
                        else
                        {
                           best[ssa_node_id] = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
                        }
                        /// and now something for the written variables
                        if(base_index && AppM->get_written_objects().find(base_index) != AppM->get_written_objects().end() && hm && hm->Rmem && function_behavior->is_variable_mem(base_index) && hm->Rmem->is_private_memory(base_index) &&
                           hm->Rmem->is_sds_var(base_index))
                        {
                           tree_nodeRef var_node = TM->get_tree_node_const(base_index);
                           if(var_node->get_kind() == var_decl_K)
                           {
                              if(private_variables.find(base_index) == private_variables.end())
                              {
                                 auto* vd = GetPointer<var_decl>(var_node);
                                 std::deque<bit_lattice> current_inf;
                                 if(vd->init)
                                 {
                                    if(GET_NODE(vd->init)->get_kind() == constructor_K)
                                    {
                                       current_inf = constructor_bitstring(GET_NODE(vd->init), ssa_node_id);
                                    }
                                    else if(GET_NODE(vd->init)->get_kind() == integer_cst_K)
                                    {
                                       auto* int_const = GetPointer<integer_cst>(GET_NODE(vd->init));
                                       current_inf = create_bitstring_from_constant(int_const->value, BitLatticeManipulator::Size(GET_NODE(vd->init)), tree_helper::is_int(TM, GET_INDEX_NODE(int_const->type)));
                                    }
                                    else if(GET_NODE(vd->init)->get_kind() == real_cst_K)
                                    {
                                       auto* real_const = GetPointer<real_cst>(GET_NODE(vd->init));
                                       const auto real_size = BitLatticeManipulator::Size(GET_CONST_NODE(real_const->type));
                                       THROW_ASSERT(real_size == 64 || real_size == 32, "Unhandled real type size (" + STR(real_size) + ")");
                                       if(real_const->valx.front() == '-' && real_const->valr.front() != real_const->valx.front())
                                       {
                                          current_inf = string_to_bitstring(convert_fp_to_string("-" + real_const->valr, real_size));
                                       }
                                       else
                                       {
                                          current_inf = string_to_bitstring(convert_fp_to_string(real_const->valr, real_size));
                                       }
                                       sign_reduce_bitstring(current_inf, false);
                                    }
                                    else if(GET_NODE(vd->init)->get_kind() == string_cst_K)
                                    {
                                       current_inf = string_cst_bitstring(GET_NODE(vd->init), ssa_node_id);
                                    }
                                    else
                                    {
                                       current_inf = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
                                    }
                                 }
                                 else
                                 {
                                    current_inf.push_back(bit_lattice::ZERO);
                                 }
                                 INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Computed the init bitstring for " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(base_index) + " = " + bitstring_to_string(current_inf));
                                 for(auto cur_var : hm->Rmem->get_source_values(base_index))
                                 {
                                    tree_nodeRef cur_node = TM->get_tree_node_const(cur_var);

                                    const bool source_is_signed = tree_helper::is_int(TM, cur_node->index);
                                    const bool loaded_is_signed = tree_helper::is_int(TM, ssa_node_id);
                                    const auto source_type = tree_helper::CGetType(cur_node);
                                    const auto source_type_size = BitLatticeManipulator::Size(source_type);
                                    INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "source node: " + STR(cur_node) + " source is signed: " + STR(source_is_signed) + " loaded is signed: " + STR(loaded_is_signed));
                                    std::deque<bit_lattice> cur_bitstring;
                                    if(cur_node->get_kind() == ssa_name_K)
                                    {
                                       const auto* ssa = GetPointer<const ssa_name>(cur_node);
                                       if(not is_handled_by_bitvalue(source_type->index))
                                       {
                                          INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "Not handled by bitvalue");
                                          cur_bitstring = create_u_bitstring(BitLatticeManipulator::Size(cur_node));
                                          INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                       }
                                       else
                                       {
                                          INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "Is handled by bitvalue");
                                          cur_bitstring = string_to_bitstring(ssa->bit_values);
                                          INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                          if(cur_bitstring.size() != 0)
                                          {
                                             INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring not empty");
                                             if(cur_bitstring.size() < source_type_size and source_is_signed != loaded_is_signed)
                                             {
                                                cur_bitstring = sign_extend_bitstring(cur_bitstring, source_is_signed, source_type_size);
                                                INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                             }
                                             sign_reduce_bitstring(cur_bitstring, loaded_is_signed);
                                             INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                          }
                                          else
                                          {
                                             INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring empty --> using U");
                                             cur_bitstring = create_u_bitstring(BitLatticeManipulator::Size(cur_node));
                                             INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                          }
                                       }
                                    }
                                    else if(cur_node->get_kind() == integer_cst_K)
                                    {
                                       INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "Integer constant");
                                       const auto* cst = GetPointer<const integer_cst>(cur_node);
                                       cur_bitstring = create_bitstring_from_constant(cst->value, source_type_size, loaded_is_signed);
                                       INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                       sign_reduce_bitstring(cur_bitstring, loaded_is_signed);
                                       INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                    }
                                    else if(cur_node->get_kind() == real_cst_K)
                                    {
                                       THROW_ASSERT(source_type_size == 64 || source_type_size == 32, "Unhandled real type size (" + STR(source_type_size) + ")");
                                       INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "Real constant");
                                       const auto* cst = GetPointer<const real_cst>(cur_node);
                                       if(cst->valx.front() == '-' && cst->valr.front() != cst->valx.front())
                                       {
                                          cur_bitstring = string_to_bitstring(convert_fp_to_string("-" + cst->valr, source_type_size));
                                       }
                                       else
                                       {
                                          cur_bitstring = string_to_bitstring(convert_fp_to_string(cst->valr, source_type_size));
                                       }
                                       sign_reduce_bitstring(cur_bitstring, loaded_is_signed);
                                       INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                    }
                                    else
                                    {
                                       cur_bitstring = create_u_bitstring(BitLatticeManipulator::Size(cur_node));
                                       INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "bitstring = " + bitstring_to_string(cur_bitstring));
                                    }
                                    current_inf = inf(current_inf, cur_bitstring, ssa_node_id);
                                    INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level, "inf = " + bitstring_to_string(current_inf));
                                 }
                                 vd->bit_values = bitstring_to_string(current_inf);
                                 INDENT_DBG_MEX(OUTPUT_LEVEL_PEDANTIC, debug_level,
                                                "Bit Value: variable " + AppM->CGetFunctionBehavior(function_id)->CGetBehavioralHelper()->PrintVariable(base_index) + " trimmed to bitsize: " + STR(vd->bit_values.size()) +
                                                    " with bit-value pattern: " + vd->bit_values);
                                 private_variables[base_index] = current_inf;
                              }
                              std::deque<bit_lattice> var_inf = private_variables.at(base_index);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Init bitstring for a private written memory variable " + bitstring_to_string(var_inf));
                              best[ssa_node_id] = var_inf;
                           }
                        }
                     }
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "=========================================");
         }
      }
   }

   /*
    * now do the real initialization on all the basic blocks
    */
   int bloc_num = 1;
   for(const auto& B_it : sl->list_of_bloc)
   {
      blocRef B = B_it.second;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "analyzing BB: " + STR(B->number));

      // first analyze statements
      int statement_num = 1;
      for(const auto& stmt : B->CGetStmtList())
      {
         // ga->op1 is equal to var_decl when it binds a newly declared variable to an ssa variable. ie. int a;
         // we can skip this assignment and focus on the ssa variable
         const auto stmt_node = GET_NODE(stmt);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "stmt " + STR(stmt_node) + " kind: " + stmt_node->get_kind_text());
         if(stmt_node->get_kind() == gimple_assign_K)
         {
            const auto* ga = GetPointer<const gimple_assign>(stmt_node);
            THROW_ASSERT(not ga->clobber, "");

            // handle lhs
            auto* lhs_ssa = GetPointer<ssa_name>(GET_NODE(ga->op0));
            if(lhs_ssa)
            {
               unsigned int ssa_node_id = lhs_ssa->index;
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "assignment index: " + STR(GET_INDEX_NODE(stmt)));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "LHS index: " + STR(ssa_node_id));
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "operation kind: " + GET_NODE(ga->op1)->get_kind_text());
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "bloc: " + STR(bloc_num) + " statement: " + STR(statement_num));

               if(not is_handled_by_bitvalue(ssa_node_id))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "variable " + STR(lhs_ssa) + " of type " + STR(tree_helper::CGetType(GET_NODE(ga->op0))) + " not considered id: " + STR(ssa_node_id));
               }
               else
               {
                  lhs_ssa->bit_values.clear();
                  bool lhs_ssa_is_signed = tree_helper::is_int(TM, ssa_node_id);
                  if(lhs_ssa_is_signed)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
                     signed_var.insert(ssa_node_id);
                  }
                  /// check if this assignment is a load from a constant array
                  if(GET_NODE(ga->op1)->get_kind() == array_ref_K || GET_NODE(ga->op1)->get_kind() == mem_ref_K || GET_NODE(ga->op1)->get_kind() == target_mem_ref_K || GET_NODE(ga->op1)->get_kind() == target_mem_ref461_K ||
                     GET_NODE(ga->op1)->get_kind() == var_decl_K)
                  {
                     // this computation was moved above, to make sure that no
                     // bitstrings (computed by previous execution of the same
                     // bitvalue analysis step) are cleared from ssa variables
                     // before computation of bitvalues for ssa read from ROMs
                     THROW_ASSERT(best.find(ssa_node_id) != best.end(), "");
                  }
                  /*if(tree_helper::is_a_pointer(TM, ssa_node_id))
                  {
                     HLS_manager*  hm = GetPointer<HLS_manager>(AppM);
                     unsigned int var = tree_helper::get_base_index(TM, ssa_node_id);
                     if(var && hm && hm->Rmem && function_behavior->is_variable_mem(var))
                        best[ssa_node_id] = create_u_bitstring(pointer_resizing(AppM, ssa_node_id, function_behavior, function_id, not_frontend, parameters));
                     else
                        best[ssa_node_id] = create_u_bitstring (BitLatticeManipulator::Size(GET_NODE(ga->op0)));
                  }*/
                  else if(GET_NODE(ga->op1)->get_kind() == call_expr_K || GET_NODE(ga->op1)->get_kind() == aggr_init_expr_K)
                  {
                     auto* ce = GetPointer<call_expr>(GET_NODE(ga->op1));
                     if(GET_NODE(ce->fn)->get_kind() == addr_expr_K)
                     {
                        const auto addr_node = GET_NODE(ce->fn);
                        const auto* ae = GetPointer<const addr_expr>(addr_node);
                        const auto fu_decl_node = GET_NODE(ae->op);
                        THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) + " is not function_decl but " + fu_decl_node->get_kind_text());
                        const tree_nodeRef ret_type_node = tree_helper::GetFunctionReturnType(fu_decl_node);
                        if(is_handled_by_bitvalue(ret_type_node->index))
                        {
                           const auto* called_fd = GetPointer<const function_decl>(fu_decl_node);
                           const auto new_bitvalue = called_fd->bit_values.empty() ? create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op0))) : string_to_bitstring(called_fd->bit_values);
                           if(best[ssa_node_id].empty())
                              best[ssa_node_id] = new_bitvalue;
                           else
                              best[ssa_node_id] = sup(new_bitvalue, best[ssa_node_id], ssa_node_id);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "updated bitstring: " + bitstring_to_string(best.at(ssa_node_id)));
                        }
                     }
                     else if(GET_NODE(ce->fn)->get_kind() != ssa_name_K)
                     {
                        THROW_UNREACHABLE("call node  " + STR(GET_NODE(ce->fn)) + " is a " + GET_NODE(ce->fn)->get_kind_text());
                     }
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == lut_expr_K)
                  {
                     best[ssa_node_id] = create_u_bitstring(1);
                  }
                  else if(GET_NODE(ga->op1)->get_kind() == extract_bit_expr_K)
                  {
                     best[ssa_node_id] = create_u_bitstring(1);
                  }
                  else
                  {
                     auto u_string = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->op0)));
                     if(lhs_ssa_is_signed && tree_helper::is_natural(TM, GET_INDEX_NODE(ga->op0)))
                     {
                        u_string.pop_front();
                        u_string.push_front(bit_lattice::ZERO);
                     }
                     best[ssa_node_id] = u_string;
                  }
               }
            }
         }
         else if(stmt_node->get_kind() == gimple_asm_K)
         {
            const auto* ga = GetPointer<const gimple_asm>(stmt_node);
            if(ga->out)
            {
               auto* lhs_ssa = GetPointer<ssa_name>(GET_NODE(ga->out));
               if(lhs_ssa)
               {
                  unsigned int ssa_node_id = lhs_ssa->index;
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "asm index: " + STR(GET_INDEX_NODE(stmt)));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "out index: " + STR(ssa_node_id));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "output ssa_name: " + GET_NODE(ga->out)->get_kind_text());
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "bloc: " + STR(bloc_num) + " statement: " + STR(statement_num));

                  if(not is_handled_by_bitvalue(ssa_node_id))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "variable " + STR(lhs_ssa) + " of type " + STR(tree_helper::CGetType(GET_NODE(ga->out))) + " not considered id: " + STR(ssa_node_id));
                  }
                  else
                  {
                     lhs_ssa->bit_values.clear();
                     if(tree_helper::is_int(TM, ssa_node_id))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
                        signed_var.insert(ssa_node_id);
                     }
                     best[ssa_node_id] = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(ga->out)));
                  }
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "not gimple_assign, nor gimple_asm: " + STR(stmt_node->index));
         }

         std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
         tree_helper::get_required_values(TM, vars_read, GET_NODE(stmt), GET_INDEX_NODE(stmt));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "requires " + STR(vars_read.size()) + " values");
         for(auto var_pair : vars_read)
         {
            unsigned int ssa_use_node_id = std::get<0>(var_pair);
            if(ssa_use_node_id == 0)
               continue;
            if(not is_handled_by_bitvalue(ssa_use_node_id))
               continue;
            tree_nodeRef use_node = TM->get_tree_node_const(ssa_use_node_id);
            auto* ssa_use = GetPointer<ssa_name>(use_node);

            if(ssa_use)
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Use: " + STR(ssa_use_node_id));
               bool ssa_is_signed = tree_helper::is_int(TM, ssa_use_node_id);
               if(ssa_is_signed)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
                  signed_var.insert(ssa_use_node_id);
               }
               const auto def = ssa_use->CGetDefStmts();
               if(def.empty())
               {
                  ssa_use->bit_values.clear();
                  best[ssa_use_node_id] = create_bitstring_from_constant(0, 1, ssa_is_signed); // create_u_bitstring(BitLatticeManipulator::Size(use_node));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "uninitialized ssa id: " + STR(ssa_use_node_id) + " new bitstring: " + bitstring_to_string(best.at(ssa_use_node_id)));
               }
               else if(ssa_use->var != nullptr and ((GET_NODE((*def.begin()))->get_kind() == gimple_nop_K) or ssa_use->volatile_flag))
               {
                  // the ssa is the first version of something
                  if(parm.find(GET_INDEX_NODE(ssa_use->var)) != parm.end())
                  {
                     // first version of a parameter
                     unsigned int parm_id = GET_INDEX_NODE(ssa_use->var);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, STR(ssa_use_node_id) + " is first version of parameter id: " + STR(parm_id));
                     arguments.insert(ssa_use_node_id);
                     ssa_use->bit_values.clear();

                     std::deque<bit_lattice> new_bitvalue = create_u_bitstring(BitLatticeManipulator::Size(use_node));
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "default bitstring: " + bitstring_to_string(new_bitvalue));
                     const auto& parm_bitvalue = parm.at(parm_id);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "param bitstring: " + bitstring_to_string(parm_bitvalue));
                     if(not parm_bitvalue.empty())
                        new_bitvalue = parm_bitvalue;
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "new bitstring: " + bitstring_to_string(new_bitvalue));

                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "id: " + STR(ssa_use_node_id) + " is in best? " + (best.find(ssa_use_node_id) == best.end() ? "NO" : "YES"));
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "previous best bitstring: " + bitstring_to_string(best[ssa_use_node_id]));
                     if(best[ssa_use_node_id].empty())
                        best[ssa_use_node_id] = new_bitvalue;
                     else
                        best[ssa_use_node_id] = sup(new_bitvalue, best[ssa_use_node_id], ssa_use_node_id);
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "updated bitstring: " + bitstring_to_string(best.at(ssa_use_node_id)));
                  }
                  else if(GET_NODE(ssa_use->var)->get_kind() == var_decl_K)
                  {
                     // first version of an uninitialized variable
                     ssa_use->bit_values.clear();
                     best[ssa_use_node_id] = create_bitstring_from_constant(0, 1, ssa_is_signed); // create_u_bitstring(BitLatticeManipulator::Size(use_node));
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "ssa id: " + STR(ssa_use_node_id) + " is the first version of uninitialized var: " + STR(GET_NODE(ssa_use->var)) + " new bitstring: " + bitstring_to_string(best.at(ssa_use_node_id)));
                  }
               }
            }
            else if(GetPointer<integer_cst>(use_node))
            {
               auto* int_const = GetPointer<integer_cst>(use_node);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Use constant: " + STR(ssa_use_node_id) + " -> " + STR(int_const->value));
               best[ssa_use_node_id] = create_bitstring_from_constant(int_const->value, BitLatticeManipulator::Size(use_node), tree_helper::is_int(TM, GET_INDEX_NODE(int_const->type)));
               if(tree_helper::is_int(TM, GET_INDEX_NODE(int_const->type)))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
                  signed_var.insert(ssa_use_node_id);
               }
               sign_reduce_bitstring(best.at(ssa_use_node_id), tree_helper::is_int(TM, ssa_use_node_id));
            }
            else if(GetPointer<real_cst>(use_node))
            {
               auto* real_const = GetPointer<real_cst>(use_node);
               const auto real_size = BitLatticeManipulator::Size(GET_CONST_NODE(real_const->type));
               THROW_ASSERT(real_size == 64 || real_size == 32, "Unhandled real type size (" + STR(real_size) + ")");
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Use constant: " + STR(ssa_use_node_id) + " -> " + STR(real_const->valr));
               if(real_const->valx.front() == '-' && real_const->valr.front() != real_const->valx.front())
               {
                  best[ssa_use_node_id] = string_to_bitstring(convert_fp_to_string("-" + real_const->valr, real_size));
               }
               else
               {
                  best[ssa_use_node_id] = string_to_bitstring(convert_fp_to_string(real_const->valr, real_size));
               }
               sign_reduce_bitstring(best.at(ssa_use_node_id), false);
            }
            else
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Use: " + STR(ssa_use_node_id));
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "=========================================");

         statement_num++;
      }

      // then analyze phis
      for(const auto& phi : B->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Phi operation");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Phi index: " + STR(GET_INDEX_NODE(phi)));
         auto* pn = GetPointer<gimple_phi>(GET_NODE(phi));
         bool is_virtual = pn->virtual_flag;
         if(not is_virtual)
         {
            unsigned int ssa_node_id = GET_INDEX_NODE(pn->res);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "LHS: " + STR(ssa_node_id));
            auto* ssa = GetPointer<ssa_name>(GET_NODE(pn->res));
            if(not is_handled_by_bitvalue(ssa_node_id))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(GET_NODE(pn->res))) + " not considered id: " + STR(ssa_node_id));
               continue;
            }
            if(tree_helper::is_int(TM, ssa_node_id))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
               signed_var.insert(ssa_node_id);
            }
            ssa->bit_values.clear();

            best[GET_INDEX_NODE(pn->res)] = create_u_bitstring(BitLatticeManipulator::Size(GET_NODE(pn->res)));

            for(const auto& def_edge : pn->CGetDefEdgesList())
            {
               if(GET_NODE(def_edge.first)->get_kind() == integer_cst_K)
               {
                  auto* int_const = GetPointer<integer_cst>(GET_NODE(def_edge.first));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Use constant: " + STR(GET_INDEX_NODE(def_edge.first)) + " -> " + STR(int_const->value));
                  best[GET_INDEX_NODE(def_edge.first)] = create_bitstring_from_constant(int_const->value, BitLatticeManipulator::Size(GET_NODE(def_edge.first)), tree_helper::is_int(TM, GET_INDEX_NODE(def_edge.first)));
                  if(tree_helper::is_int(TM, GET_INDEX_NODE(int_const->type)))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
                     signed_var.insert(GET_INDEX_NODE(def_edge.first));
                  }
                  sign_reduce_bitstring(best.at(GET_INDEX_NODE(def_edge.first)), tree_helper::is_int(TM, GET_INDEX_NODE(def_edge.first)));
               }
               else if(GET_NODE(def_edge.first)->get_kind() == real_cst_K)
               {
                  auto* real_const = GetPointer<real_cst>(GET_NODE(def_edge.first));
                  const auto real_size = BitLatticeManipulator::Size(GET_CONST_NODE(real_const->type));
                  THROW_ASSERT(real_size == 64 || real_size == 32, "Unhandled real type size (" + STR(real_size) + ")");
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Use constant: " + STR(GET_INDEX_NODE(def_edge.first)) + " -> " + STR(real_const->valr));
                  if(real_const->valx.front() == '-' && real_const->valr.front() != real_const->valx.front())
                  {
                     best[GET_INDEX_NODE(def_edge.first)] = string_to_bitstring(convert_fp_to_string("-" + real_const->valr, real_size));
                  }
                  else
                  {
                     best[GET_INDEX_NODE(def_edge.first)] = string_to_bitstring(convert_fp_to_string(real_const->valr, real_size));
                  }
                  sign_reduce_bitstring(best.at(GET_INDEX_NODE(def_edge.first)), false);
               }
               else
               {
                  unsigned int ssa_use_node_id = GET_INDEX_NODE(def_edge.first);
                  const auto use_node = GET_NODE(def_edge.first);
                  auto* ssa_use = GetPointer<ssa_name>(use_node);
                  if(not is_handled_by_bitvalue(ssa_use_node_id))
                     continue;

                  if(ssa_use)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Use: " + STR(ssa_use_node_id));
                     bool ssa_is_signed = tree_helper::is_int(TM, ssa_use_node_id);
                     if(ssa_is_signed)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "is signed");
                        signed_var.insert(ssa_use_node_id);
                     }
                     const auto def = ssa_use->CGetDefStmts();
                     if(def.empty())
                     {
                        ssa_use->bit_values.clear();
                        best[ssa_use_node_id] = create_bitstring_from_constant(0, 1, ssa_is_signed); // create_u_bitstring(BitLatticeManipulator::Size(use_node));
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "uninitialized ssa id: " + STR(ssa_use_node_id) + " new bitstring: " + bitstring_to_string(best.at(ssa_use_node_id)));
                     }
                     else if(ssa_use->var != nullptr and ((GET_NODE((*def.begin()))->get_kind() == gimple_nop_K) or ssa_use->volatile_flag))
                     {
                        // the ssa is the first version of something
                        if(parm.find(GET_INDEX_NODE(ssa_use->var)) != parm.end())
                        {
                           // first version of a parameter
                           unsigned int parm_id = GET_INDEX_NODE(ssa_use->var);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, STR(ssa_use_node_id) + " is first version of parameter id: " + STR(parm_id));
                           arguments.insert(ssa_use_node_id);
                           ssa_use->bit_values.clear();

                           std::deque<bit_lattice> new_bitvalue = create_u_bitstring(BitLatticeManipulator::Size(use_node));
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "default bitstring: " + bitstring_to_string(new_bitvalue));
                           const auto& parm_bitvalue = parm.at(parm_id);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "param bitstring: " + bitstring_to_string(parm_bitvalue));
                           if(not parm_bitvalue.empty())
                              new_bitvalue = parm_bitvalue;
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "new bitstring: " + bitstring_to_string(new_bitvalue));

                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "id: " + STR(ssa_use_node_id) + " is in best? " + (best.find(ssa_use_node_id) == best.end() ? "NO" : "YES"));
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "previous best bitstring: " + bitstring_to_string(best[ssa_use_node_id]));
                           if(best[ssa_use_node_id].empty())
                              best[ssa_use_node_id] = new_bitvalue;
                           else
                              best[ssa_use_node_id] = sup(new_bitvalue, best[ssa_use_node_id], ssa_use_node_id);
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "updated bitstring: " + bitstring_to_string(best.at(ssa_use_node_id)));
                        }
                        else if(GET_NODE(ssa_use->var)->get_kind() == var_decl_K)
                        {
                           // first version of an uninitialized variable
                           ssa_use->bit_values.clear();
                           best[ssa_use_node_id] = create_bitstring_from_constant(0, 1, ssa_is_signed); // create_u_bitstring(BitLatticeManipulator::Size(use_node));
                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "ssa id: " + STR(ssa_use_node_id) + " is the first version of uninitialized var: " + STR(GET_NODE(ssa_use->var)) + " new bitstring: " + bitstring_to_string(best.at(ssa_use_node_id)));
                        }
                     }
                  }
                  else
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Use: " + STR(ssa_use_node_id));
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "=========================================");
         }
      }
      bloc_num++;
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initialized best with all variables in the functions:");
   print_bitstring_map(best);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended internal initialize");
}

void Bit_Value::clear_current()
{
   for(const auto& b : best)
   {
      const auto position_in_current = current.find(b.first);
      if(position_in_current != current.end())
      {
         current.erase(position_in_current);
      }
      if(arguments.find(b.first) != arguments.end() || b.first == function_id)
      {
         current[b.first] = b.second;
      }
   }
}

DesignFlowStep_Status Bit_Value::InternalExec()
{
   initialize();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing initial backward");
   backward();
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed initial backward");
   mix();
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at the end of initial backward:");
   print_bitstring_map(best);
   PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
   bool restart;
   do
   {
      clear_current();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing forward");
      forward();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed forward");
      mix();
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at the end of forward:");
      print_bitstring_map(best);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Performing backward");
      backward();
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Performed backward");
      restart = mix();
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at end of backward:");
      print_bitstring_map(best);
      PRINT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "");
   } while(restart);
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at the end of alg:");
   print_bitstring_map(best);
   auto changed = update_IR();
   BitLatticeManipulator::clear();
   direct_call_id_to_called_id.clear();
   arguments.clear();
   if(changed)
   {
      bitvalue_version = function_behavior->UpdateBitValueVersion();
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

void Bit_Value::Initialize()
{
   const std::string bambu_frontend_flow_signature = ApplicationFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BAMBU_FRONTEND_FLOW);
   not_frontend = design_flow_manager.lock()->GetStatus(bambu_frontend_flow_signature) == DesignFlowStep_Status::EMPTY;
}

bool Bit_Value::HasToBeExecuted() const
{
   return (bitvalue_version != function_behavior->GetBitValueVersion()) or FunctionFrontendFlowStep::HasToBeExecuted();
}
