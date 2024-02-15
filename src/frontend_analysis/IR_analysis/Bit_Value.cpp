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
 *              Copyright (C) 2004-2024 Politecnico di Milano
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
 * Mihai Budiu, Majd Sakr, Kip Walker, Seth Copen Goldstein: BitValue Inference: Detecting and Exploiting Narrow
 * Bitwidth Computations. Euro-Par 2000: 969-979
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
#include "Dominance.hpp"
#include "Parameter.hpp"
#include "basic_block.hpp"

/// behavior includes
#include "application_manager.hpp"
#include "call_graph.hpp"
#include "call_graph_manager.hpp"
#include "function_behavior.hpp"
#include "op_graph.hpp"

/// design_flows include
#include "design_flow_manager.hpp"

/// frontend_analysis
#include "Range.hpp"
#include "application_frontend_flow_step.hpp"

/// HLS include
#include "hls_device.hpp"
#include "hls_manager.hpp"

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

/// wrapper/compiler include
#include "compiler_wrapper.hpp"
#include "string_manipulation.hpp" // for GET_CLASS

const std::map<bit_lattice, std::map<bit_lattice, std::map<bit_lattice, std::deque<bit_lattice>>>>
    Bit_Value::plus_expr_map = {
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

const std::map<bit_lattice, std::map<bit_lattice, std::map<bit_lattice, std::deque<bit_lattice>>>>
    Bit_Value::minus_expr_map = {
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

unsigned long long Bit_Value::pointer_resizing(unsigned int output_id) const
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level,
                  "-->Pointer resizing starting from " + TM->CGetTreeNode(output_id)->ToString());
   unsigned int var = tree_helper::get_base_index(TM, output_id);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Base variable is " + TM->CGetTreeNode(var)->ToString());
   unsigned long long address_bitsize;
   if(not_frontend)
   {
      auto* hm = GetPointer<HLS_manager>(AppM);
      if(hm and hm->Rmem)
      {
         if(var and function_behavior->is_variable_mem(var))
         {
            unsigned long long int max_addr =
                hm->Rmem->get_base_address(var, function_id) + tree_helper::TypeSize(TM->CGetTreeNode(var)) / 8;
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "Maximum address is " + STR(max_addr - 1));
            for(address_bitsize = 1; max_addr > (1ull << address_bitsize); ++address_bitsize)
            {
               ;
            }
            /// added to consider the 64bit alignment of the allocated variables
            if(address_bitsize < 4)
            {
               address_bitsize = 4;
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
               {
                  if((1ULL << index) & align)
                  {
                     found = true;
                     break;
                  }
               }
               if(!found)
               {
                  ++address_bitsize;
               }
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
      address_bitsize = static_cast<unsigned int>(CompilerWrapper::CGetPointerSize(parameters));
      INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Pointer bitsize " + STR(address_bitsize));
   }
   return address_bitsize;
}

unsigned int Bit_Value::lsb_to_zero(const addr_expr* ae, bool safe) const
{
   const auto vd = GetPointer<const var_decl>(GET_CONST_NODE(ae->op));
   if(!vd)
   {
      return 0;
   }
   auto align = vd->algn;
   if(safe)
   {
      align = 1;
   }
   else
   {
      align = align < 64 ? 8 : (align / 8);
   }
   auto index = 0u;
   bool found = false;
   for(; index < AppM->get_address_bitsize(); ++index)
   {
      if((1ULL << index) & align)
      {
         found = true;
         break;
      }
   }
   return found ? index : 0;
}

Bit_Value::Bit_Value(const ParameterConstRef params, const application_managerRef AM, unsigned int f_id,
                     const DesignFlowManagerConstRef dfm)
    : FunctionFrontendFlowStep(AM, f_id, BIT_VALUE, dfm, params),
      BitLatticeManipulator(AM->get_tree_manager(), parameters->get_class_debug_level(GET_CLASS(*this))),
      not_frontend(false)
{
   debug_level = parameters->get_class_debug_level(GET_CLASS(*this), DEBUG_LEVEL_NONE);
}

Bit_Value::~Bit_Value() = default;

const CustomUnorderedSet<std::pair<FrontendFlowStepType, FrontendFlowStep::FunctionRelationship>>
Bit_Value::ComputeFrontendRelationships(const DesignFlowStep::RelationshipType relationship_type) const
{
   CustomUnorderedSet<std::pair<FrontendFlowStepType, FunctionRelationship>> relationships;
   switch(relationship_type)
   {
      case DEPENDENCE_RELATIONSHIP:
      {
         relationships.insert(std::make_pair(BIT_VALUE, CALLED_FUNCTIONS));
         relationships.insert(std::make_pair(CALL_GRAPH_BUILTIN_CALL, SAME_FUNCTION));
         relationships.insert(std::make_pair(COMPUTE_IMPLICIT_CALLS, SAME_FUNCTION));
         relationships.insert(std::make_pair(DETERMINE_MEMORY_ACCESSES, SAME_FUNCTION));
         relationships.insert(std::make_pair(ESSA, SAME_FUNCTION));
         relationships.insert(std::make_pair(EXTRACT_GIMPLE_COND_OP, SAME_FUNCTION));
         relationships.insert(std::make_pair(FIX_STRUCTS_PASSED_BY_VALUE, SAME_FUNCTION));
         relationships.insert(std::make_pair(FUNCTION_CALL_TYPE_CLEANUP, SAME_FUNCTION));
         relationships.insert(std::make_pair(IR_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(PARM2SSA, SAME_FUNCTION));
         if(parameters->isOption(OPT_soft_float) && parameters->getOption<bool>(OPT_soft_float))
         {
            relationships.insert(std::make_pair(SOFT_FLOAT_CG_EXT, SAME_FUNCTION));
         }
         relationships.insert(std::make_pair(UN_COMPARISON_LOWERING, SAME_FUNCTION));
         relationships.insert(std::make_pair(USE_COUNTING, SAME_FUNCTION));
         break;
      }
      case(PRECEDENCE_RELATIONSHIP):
      {
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

bool Bit_Value::HasToBeExecuted() const
{
   return (bitvalue_version != function_behavior->GetBitValueVersion()) or FunctionFrontendFlowStep::HasToBeExecuted();
}

void Bit_Value::Initialize()
{
   const auto bambu_frontend_flow_signature =
       ApplicationFrontendFlowStep::ComputeSignature(FrontendFlowStepType::BAMBU_FRONTEND_FLOW);
   not_frontend = design_flow_manager.lock()->GetStatus(bambu_frontend_flow_signature) == DesignFlowStep_Status::EMPTY;

   const auto tn = TM->CGetTreeNode(function_id);
   THROW_ASSERT(tn->get_kind() == function_decl_K, "Node is not a function");
   const auto fd = GetPointerS<const function_decl>(tn);
   THROW_ASSERT(fd->body, "Function has not a body");
   const auto sl = GetPointerS<const statement_list>(GET_CONST_NODE(fd->body));
   /// store the IR BB graph ala boost::graph
   BBGraphsCollectionRef bb_graphs_collection(
       new BBGraphsCollection(BBGraphInfoRef(new BBGraphInfo(AppM, function_id)), parameters));
   BBGraph cfg(bb_graphs_collection, CFG_SELECTOR);
   CustomUnorderedMap<unsigned int, vertex> inverse_vertex_map;
   /// add vertices
   for(const auto& block : sl->list_of_bloc)
   {
      inverse_vertex_map.insert(
          std::make_pair(block.first, bb_graphs_collection->AddVertex(BBNodeInfoRef(new BBNodeInfo(block.second)))));
   }

   /// add edges
   for(const auto& curr_bb_pair : sl->list_of_bloc)
   {
      const auto curr_bbi = curr_bb_pair.first;
      const auto curr_bb = curr_bb_pair.second;
      for(const auto& lop : curr_bb->list_of_pred)
      {
         THROW_ASSERT(static_cast<bool>(inverse_vertex_map.count(lop)),
                      "BB" + STR(lop) + " (successor of BB" + STR(curr_bbi) + ") does not exist");
         bb_graphs_collection->AddEdge(inverse_vertex_map.at(lop), inverse_vertex_map.at(curr_bbi), CFG_SELECTOR);
      }

      for(const auto& los : curr_bb->list_of_succ)
      {
         if(los == bloc::EXIT_BLOCK_ID)
         {
            bb_graphs_collection->AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(los), CFG_SELECTOR);
         }
      }

      if(curr_bb->list_of_succ.empty())
      {
         bb_graphs_collection->AddEdge(inverse_vertex_map.at(curr_bbi), inverse_vertex_map.at(bloc::EXIT_BLOCK_ID),
                                       CFG_SELECTOR);
      }
   }
   /// add a connection between entry and exit thus avoiding problems with non terminating code
   bb_graphs_collection->AddEdge(inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID),
                                 inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), CFG_SELECTOR);
   dominance<BBGraph> bb_dominators(cfg, inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID),
                                    inverse_vertex_map.at(bloc::EXIT_BLOCK_ID), parameters);
   bb_dominators.calculate_dominance_info(dominance<BBGraph>::CDI_DOMINATORS);

   BBGraphRef dt(new BBGraph(bb_graphs_collection, D_SELECTOR));
   for(const auto& it : bb_dominators.get_dominator_map())
   {
      if(it.first != inverse_vertex_map.at(bloc::ENTRY_BLOCK_ID))
      {
         bb_graphs_collection->AddEdge(it.second, it.first, D_SELECTOR);
      }
   }
   dt->GetBBGraphInfo()->bb_index_map = std::move(inverse_vertex_map);

   std::list<vertex> v_topological;
   dt->TopologicalSort(v_topological);
   THROW_ASSERT(v_topological.size(), "");
   bb_topological.reserve(v_topological.size());
   std::transform(v_topological.begin(), v_topological.end(), std::back_inserter(bb_topological),
                  [&](const vertex& v) { return dt->CGetBBNodeInfo(v)->block; });
}

DesignFlowStep_Status Bit_Value::InternalExec()
{
   if(parameters->IsParameter("bitvalue") && !parameters->GetParameter<unsigned int>("bitvalue"))
   {
      return DesignFlowStep_Status::UNCHANGED;
   }
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
   bb_topological.clear();
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "best at the end of alg:");
   print_bitstring_map(best);
   auto changed = update_IR();
   BitLatticeManipulator::clear();
   direct_call_id_to_called_id.clear();
   arguments.clear();
   if(changed)
   {
      function_behavior->UpdateBitValueVersion();
   }
   return changed ? DesignFlowStep_Status::SUCCESS : DesignFlowStep_Status::UNCHANGED;
}

// prints the content of a bitstring map
void Bit_Value::print_bitstring_map(const CustomMap<unsigned int, std::deque<bit_lattice>>&
#ifndef NDEBUG
                                        map
#endif
) const
{
#ifndef NDEBUG
   const auto BH = function_behavior->CGetBehavioralHelper();
   for(const auto& m : map)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "var_uid: " + STR(m.first) + ":" + BH->PrintVariable(m.first) +
                         " bitstring: " + bitstring_to_string(m.second));
   }
#endif
}

bool Bit_Value::update_IR()
{
   bool res = false;
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Updating IR");
   for(const auto& b : best)
   {
      const auto tn_id = b.first;
      const auto tn = TM->GetTreeNode(tn_id);
      const auto kind = tn->get_kind();
      if(kind == ssa_name_K)
      {
         auto ssa = GetPointerS<ssa_name>(tn);
         if(ssa->bit_values.empty() || isBetter(bitstring_to_string(b.second), ssa->bit_values))
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "Variable: " + ssa->ToString() + " bitstring: " + ssa->bit_values + " -> " +
                               bitstring_to_string(b.second));
            ssa->bit_values = bitstring_to_string(b.second);
            // ssa->range = Range::fromBitValues(b.second, static_cast<Range::bw_t>(tree_helper::TypeSize(tn)),
            // signed_var.count(ssa->index));
            res = true;
            AppM->RegisterTransformation(GetName(), tn);
         }
      }
      else if(kind == function_decl_K)
      {
         auto fd = GetPointerS<function_decl>(tn);
         if(fd->bit_values.empty() || isBetter(bitstring_to_string(b.second), fd->bit_values))
         {
            if(!AppM->ApplyNewTransformation())
            {
               break;
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "Function: " + tree_helper::name_function(TM, b.first) + " bitstring: " + fd->bit_values +
                               " -> " + bitstring_to_string(b.second));
            fd->bit_values = bitstring_to_string(b.second);
            res = true;
            AppM->RegisterTransformation(GetName(), tn);
         }
      }
      else if(kind == integer_cst_K || kind == real_cst_K)
      {
         // do nothing, constants are recomputed every time
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

   const auto CGMan = AppM->CGetCallGraphManager();
   const auto cg = CGMan->CGetCallGraph();
   const auto v = CGMan->GetVertex(function_id);

   const auto rbf = CGMan->GetReachedBodyFunctions();
   OutEdgeIterator oe_it, oe_end;
   boost::tie(oe_it, oe_end) = boost::out_edges(v, *cg);
   for(; oe_it != oe_end; oe_it++)
   {
      const auto call_edge_info = cg->CGetFunctionEdgeInfo(*oe_it);
      for(const auto& i : call_edge_info->direct_call_points)
      {
         const auto called_id = CGMan->get_function(boost::target(*oe_it, *cg));
         if(i == 0)
         {
            // never analyze artificial calls
            THROW_ASSERT(AppM->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->get_function_name() == MEMCPY,
                         "function " + function_behavior->CGetBehavioralHelper()->get_function_name() +
                             " calls function " +
                             AppM->CGetFunctionBehavior(called_id)->CGetBehavioralHelper()->get_function_name() +
                             " with an artificial call: this should not happen");
            continue;
         }
         if(rbf.find(called_id) != rbf.end())
         {
            direct_call_id_to_called_id[i] = called_id;
         }
      }
   }

   const auto tn = TM->CGetTreeNode(function_id);
   const auto fd = GetPointerS<const function_decl>(tn);

   /*
    * loop on the list of arguments and initialize best
    */
   for(const auto& parm_decl_node : fd->list_of_args)
   {
      const auto parmssa_id = AppM->getSSAFromParm(function_id, GET_INDEX_CONST_NODE(parm_decl_node));
      const auto parm_type = tree_helper::CGetType(parm_decl_node);
      if(!IsHandledByBitvalue(parm_type))
      {
         continue;
      }
      const auto parmssa = TM->CGetTreeNode(parmssa_id);
      const auto p = GetPointerS<const ssa_name>(parmssa);
      const auto b = p->CGetUseStmts().empty() ?
                         create_x_bitstring(1) :
                         (p->bit_values.empty() ? create_u_bitstring(tree_helper::TypeSize(parmssa)) :
                                                  string_to_bitstring(p->bit_values));
      best[parmssa_id] = b;
      arguments.insert(parmssa_id);
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "Parameter " + STR(GET_CONST_NODE(parm_decl_node)) + "(" +
                         STR(GET_INDEX_CONST_NODE(parm_decl_node)) + ") bound to " + parmssa->ToString() + ": " +
                         bitstring_to_string(b) + "");
   }

   /*
    * initialize the bitvalue strings for the return value that have been set on
    * the function_decl from the BitValueIPA
    */
   const auto fu_type = tree_helper::CGetType(tn);
   const auto fret_type_node = tree_helper::GetFunctionReturnType(fu_type);

   if(!fret_type_node || !IsHandledByBitvalue(fret_type_node))
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                     "Function returning " + (fret_type_node ? STR(fret_type_node) : "void") + " not considered");
   }
   else
   {
      const auto is_signed = tree_helper::IsSignedIntegerType(fret_type_node);
      if(fd->bit_values.empty())
      {
         best[function_id] =
             fd->range ? fd->range->getBitValues(is_signed) : create_u_bitstring(tree_helper::TypeSize(fret_type_node));
      }
      else
      {
         best[function_id] = string_to_bitstring(fd->bit_values);
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Return value: " + bitstring_to_string(best.at(function_id)));
      if(is_signed)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
         signed_var.insert(function_id);
      }
   }

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
      CustomMap<unsigned int, std::deque<bit_lattice>> private_variables;
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initializing ROMs loaded ssa bitvalues");
      for(const auto& B : bb_topological)
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzing BB" + STR(B->number));
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->");
         for(const auto& stmt : B->CGetStmtList())
         {
            const auto stmt_node = GET_CONST_NODE(stmt);
            if(stmt_node->get_kind() == gimple_assign_K)
            {
               const auto ga = GetPointerS<const gimple_assign>(stmt_node);
               THROW_ASSERT(!ga->clobber, "");
               const auto lhs = GET_CONST_NODE(ga->op0);
               // handle lhs
               if(lhs->get_kind() == ssa_name_K)
               {
                  const auto lhs_nid = GET_INDEX_CONST_NODE(ga->op0);
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "Analyzing " + stmt_node->get_kind_text() + "(" + STR(stmt_node->index) +
                                     "): " + STR(stmt_node));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---LHS: " + STR(lhs));
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---RHS: " + GET_CONST_NODE(ga->op1)->get_kind_text());

                  if(!IsHandledByBitvalue(lhs))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---variable " + STR(GetPointerS<const ssa_name>(lhs)) + " of type " +
                                        STR(tree_helper::CGetType(lhs)) + " not considered");
                  }
                  else
                  {
                     const auto lhs_signed = tree_helper::IsSignedIntegerType(lhs);
                     if(lhs_signed)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
                        signed_var.insert(lhs_nid);
                     }
                     /// check if this assignment is a load from a constant array
                     const auto ga_op1_kind = GET_CONST_NODE(ga->op1)->get_kind();
                     if(ga_op1_kind == array_ref_K || ga_op1_kind == mem_ref_K || ga_op1_kind == target_mem_ref_K ||
                        ga_op1_kind == target_mem_ref461_K || ga_op1_kind == var_decl_K)
                     {
                        const auto hm = GetPointer<const HLS_manager>(AppM);
                        const auto base_index = tree_helper::get_base_index(TM, GET_INDEX_CONST_NODE(ga->op1));
                        const auto var_node = TM->GetTreeNode(base_index);
                        auto vd = GetPointer<var_decl>(var_node);
                        if(base_index &&
                           AppM->get_written_objects().find(base_index) == AppM->get_written_objects().end() && hm &&
                           hm->Rmem && hm->Rmem->get_enable_hls_bit_value() &&
                           function_behavior->is_variable_mem(base_index) && hm->Rmem->is_sds_var(base_index) && vd &&
                           vd->init)
                        {
                           std::deque<bit_lattice> current_inf;
                           if(GET_CONST_NODE(vd->init)->get_kind() == constructor_K)
                           {
                              current_inf = constructor_bitstring(GET_CONST_NODE(vd->init), lhs_nid);
                           }
                           else if(GET_CONST_NODE(vd->init)->get_kind() == integer_cst_K)
                           {
                              const auto cst_val = tree_helper::GetConstValue(vd->init);
                              current_inf = create_bitstring_from_constant(
                                  cst_val, tree_helper::TypeSize(GET_CONST_NODE(vd->init)),
                                  tree_helper::IsSignedIntegerType(vd->init));
                           }
                           else if(GET_CONST_NODE(vd->init)->get_kind() == string_cst_K)
                           {
                              current_inf = string_cst_bitstring(GET_CONST_NODE(vd->init), lhs_nid);
                           }
                           else
                           {
                              current_inf = create_u_bitstring(tree_helper::TypeSize(lhs));
                           }

                           INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                          "---Used the init bitstring " + bitstring_to_string(current_inf));

                           vd->bit_values = bitstring_to_string(current_inf);
                           best[lhs_nid] = current_inf;
                        }
                        else
                        {
                           best[lhs_nid] = create_u_bitstring(tree_helper::TypeSize(lhs));
                        }
                        /// and now something for the written variables
                        if(base_index &&
                           AppM->get_written_objects().find(base_index) != AppM->get_written_objects().end() && hm &&
                           hm->Rmem && hm->Rmem->get_enable_hls_bit_value() &&
                           function_behavior->is_variable_mem(base_index) && hm->Rmem->is_private_memory(base_index) &&
                           hm->Rmem->is_sds_var(base_index))
                        {
                           if(vd)
                           {
                              if(!private_variables.count(base_index))
                              {
                                 std::deque<bit_lattice> current_inf;
                                 if(vd->init)
                                 {
                                    if(GET_CONST_NODE(vd->init)->get_kind() == constructor_K)
                                    {
                                       current_inf = constructor_bitstring(GET_CONST_NODE(vd->init), lhs_nid);
                                    }
                                    else if(GET_CONST_NODE(vd->init)->get_kind() == integer_cst_K)
                                    {
                                       const auto cst_val = tree_helper::GetConstValue(vd->init);
                                       current_inf = create_bitstring_from_constant(
                                           cst_val, tree_helper::TypeSize(GET_CONST_NODE(vd->init)),
                                           tree_helper::IsSignedIntegerType(vd->init));
                                    }
                                    else if(GET_CONST_NODE(vd->init)->get_kind() == string_cst_K)
                                    {
                                       current_inf = string_cst_bitstring(GET_CONST_NODE(vd->init), lhs_nid);
                                    }
                                    else
                                    {
                                       current_inf = create_u_bitstring(tree_helper::TypeSize(lhs));
                                    }
                                 }
                                 else
                                 {
                                    current_inf.push_back(bit_lattice::X);
                                 }
                                 INDENT_DBG_MEX(
                                     DEBUG_LEVEL_PEDANTIC, debug_level,
                                     "---Computed the init bitstring for " +
                                         function_behavior->CGetBehavioralHelper()->PrintVariable(base_index) + " = " +
                                         bitstring_to_string(current_inf));
                                 for(const auto& cur_var : hm->Rmem->get_source_values(base_index))
                                 {
                                    const auto cur_node = TM->CGetTreeNode(cur_var);
                                    const auto source_is_signed = tree_helper::IsSignedIntegerType(cur_node);
                                    const auto source_type = tree_helper::CGetType(cur_node);
                                    const auto source_type_size = tree_helper::TypeSize(source_type);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                   "---source node: " + STR(cur_node) + " source is signed: " +
                                                       STR(source_is_signed) + " loaded is signed: " + STR(lhs_signed));
                                    std::deque<bit_lattice> cur_bitstring;
                                    if(cur_node->get_kind() == ssa_name_K)
                                    {
                                       const auto ssa = GetPointerS<const ssa_name>(cur_node);
                                       if(!IsHandledByBitvalue(source_type))
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                         "---Not handled by bitvalue");
                                          cur_bitstring = create_u_bitstring(tree_helper::TypeSize(cur_node));
                                       }
                                       else
                                       {
                                          INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                         "---Is handled by bitvalue");
                                          cur_bitstring = string_to_bitstring(ssa->bit_values);
                                       }
                                    }
                                    else if(cur_node->get_kind() == integer_cst_K)
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---Integer constant");
                                       const auto cst_val = tree_helper::GetConstValue(cur_node);
                                       cur_bitstring =
                                           create_bitstring_from_constant(cst_val, source_type_size, lhs_signed);
                                    }
                                    else
                                    {
                                       cur_bitstring = create_u_bitstring(tree_helper::TypeSize(cur_node));
                                    }
                                    if(cur_bitstring.size() != 0)
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                      "---bitstring = " + bitstring_to_string(cur_bitstring));
                                       if(cur_bitstring.size() < source_type_size && source_is_signed != lhs_signed)
                                       {
                                          cur_bitstring =
                                              sign_extend_bitstring(cur_bitstring, source_is_signed, source_type_size);
                                          INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                         "---bitstring = " + bitstring_to_string(cur_bitstring));
                                       }
                                       sign_reduce_bitstring(cur_bitstring, lhs_signed);
                                       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                      "---bitstring = " + bitstring_to_string(cur_bitstring));
                                    }
                                    else
                                    {
                                       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                      "---bitstring empty --> using U");
                                       cur_bitstring = create_u_bitstring(tree_helper::TypeSize(cur_node));
                                       INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                      "---bitstring = " + bitstring_to_string(cur_bitstring));
                                    }
                                    current_inf = inf(current_inf, cur_bitstring, lhs);
                                    INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                                   "---inf = " + bitstring_to_string(current_inf));
                                 }
                                 while(current_inf.front() == bit_lattice::X)
                                 {
                                    current_inf.pop_front();
                                 }
                                 if(current_inf.empty())
                                 {
                                    current_inf.push_back(bit_lattice::ZERO);
                                 }
                                 THROW_ASSERT(std::find(current_inf.begin(), current_inf.end(), bit_lattice::X) ==
                                                  current_inf.end(),
                                              "Init bitstring must not contain X: " + bitstring_to_string(current_inf));
                                 vd->bit_values = bitstring_to_string(current_inf);
                                 INDENT_DBG_MEX(
                                     DEBUG_LEVEL_PEDANTIC, debug_level,
                                     "---Bit Value: variable " +
                                         function_behavior->CGetBehavioralHelper()->PrintVariable(base_index) +
                                         " trimmed to bitsize: " + STR(vd->bit_values.size()) +
                                         " with bit-value pattern: " + vd->bit_values);
                                 private_variables[base_index] = current_inf;
                              }
                              const auto var_inf = private_variables.at(base_index);
                              INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                             "---Init bitstring for a private written memory variable " +
                                                 bitstring_to_string(var_inf));
                              best[lhs_nid] = var_inf;
                           }
                        }
                     }
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "Analyzed " + stmt_node->get_kind_text() + ": " + STR(stmt_node));
               }
            }
            else if(stmt_node->get_kind() == gimple_asm_K)
            {
               const auto ga = GetPointerS<const gimple_asm>(stmt_node);
               if(ga->out)
               {
                  const auto tl = GetPointer<const tree_list>(GET_CONST_NODE(ga->out));
                  THROW_ASSERT(tl->valu, "only the first output and so only single output gimple_asm are supported");
                  const auto ssa = GetPointer<const ssa_name>(GET_CONST_NODE(tl->valu));
                  if(ssa && !ssa->CGetUseStmts().empty() && IsHandledByBitvalue(tl->valu))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "Analyzing " + stmt_node->get_kind_text() + "(" + STR(stmt_node->index) + ")");
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---Initializing bitstring for an asm instruction");
                     best[GET_INDEX_CONST_NODE(tl->valu)] =
                         create_u_bitstring(tree_helper::TypeSize(GET_CONST_NODE(tl->valu)));
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzed " + stmt_node->get_kind_text());
                  }
               }
            }
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzed BB" + STR(B->number));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initialized ROMs loaded ssa bitvalues");
   }

   const auto set_value = [&](unsigned int node_id) {
      if(node_id == 0)
      {
         return;
      }
      auto use_node = TM->GetTreeNode(node_id);
      if(!IsHandledByBitvalue(use_node))
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---variable " + STR(use_node) + " of type " + STR(tree_helper::CGetType(use_node)) +
                            " not considered");
         return;
      }

      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Use: " + STR(use_node));
      if(use_node->get_kind() == ssa_name_K)
      {
         const auto ssa = GetPointerS<ssa_name>(use_node);
         const auto ssa_is_signed = tree_helper::IsSignedIntegerType(use_node);
         if(ssa_is_signed)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
            signed_var.insert(node_id);
         }
         if(ssa->volatile_flag)
         {
            ssa->bit_values.clear();
            best[node_id] = create_u_bitstring(tree_helper::TypeSize(use_node));
         }
         else
         {
            const auto def = ssa->CGetDefStmt();
            if(!def || (ssa->var != nullptr && ((GET_CONST_NODE(def)->get_kind() == gimple_nop_K)) &&
                        GET_CONST_NODE(ssa->var)->get_kind() == var_decl_K))
            {
               ssa->bit_values.clear();
               best[node_id] = create_bitstring_from_constant(0, 1, ssa_is_signed);
               if(ssa->var)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---first version of uninitialized var " + STR(GET_CONST_NODE(ssa->var)));
               }
               else
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---uninitialized ssa");
               }
               if(AppM->ApplyNewTransformation())
               {
                  const auto tree_man = AppM->get_tree_manager();
                  const auto ssa_node = tree_man->GetTreeReindex(node_id);
                  const auto cst_value = tree_man->CreateUniqueIntegerCst(0, ssa->type);
                  const auto uses = ssa->CGetUseStmts();
                  for(const auto& stmt_use : uses)
                  {
                     tree_man->ReplaceTreeNode(stmt_use.first, ssa_node, cst_value);
                  }
                  AppM->RegisterTransformation(GetName(), ssa_node);
                  use_node = GET_NODE(cst_value);
                  node_id = GET_INDEX_NODE(cst_value);
               }
            }
         }
      }

      if(use_node->get_kind() == integer_cst_K)
      {
         const auto cst_val = tree_helper::GetConstValue(use_node);
         const auto is_signed = tree_helper::IsSignedIntegerType(use_node);
         best[node_id] = create_bitstring_from_constant(cst_val, tree_helper::TypeSize(use_node), is_signed);
         if(is_signed)
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
            signed_var.insert(node_id);
         }
         sign_reduce_bitstring(best.at(node_id), is_signed);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "---updated bitstring: " + bitstring_to_string(best.at(node_id)));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
   };

   /*
    * now do the real initialization on all the basic blocks
    */
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initializing all ssa bitvalues");
   for(const auto& B : bb_topological)
   {
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzing BB" + STR(B->number));
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->");

      for(const auto& phi : B->CGetPhiList())
      {
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "Analyzing phi(" + STR(GET_INDEX_CONST_NODE(phi)) + "): " + STR(phi));
         const auto pn = GetPointerS<const gimple_phi>(GET_CONST_NODE(phi));
         const auto is_virtual = pn->virtual_flag;
         if(!is_virtual)
         {
            const auto res_nid = GET_INDEX_CONST_NODE(pn->res);
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---LHS: " + STR(res_nid));
            auto ssa = GetPointer<ssa_name>(GET_NODE(pn->res));
            THROW_ASSERT(ssa, "unexpected condition");
            if(!IsHandledByBitvalue(pn->res))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                              "---variable " + STR(ssa) + " of type " + STR(tree_helper::CGetType(pn->res)) +
                                  " not considered id: " + STR(res_nid));
               continue;
            }
            if(tree_helper::IsSignedIntegerType(pn->res))
            {
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
               signed_var.insert(res_nid);
            }
            if(bb_version == 0 || bb_version != function_behavior->GetBBVersion())
            {
               ssa->bit_values.clear();
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---bit_values cleared : " + STR(ssa));
            }

            if(ssa->CGetUseStmts().empty())
            {
               best[res_nid] = create_x_bitstring(1);
               if(best.count(res_nid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---updated bitstring: " + bitstring_to_string(best.at(res_nid)));
               }
            }
            else
            {
               if(ssa->bit_values.empty())
               {
                  best[res_nid] = create_u_bitstring(tree_helper::TypeSize(pn->res));
               }
               else
               {
                  best[res_nid] = string_to_bitstring(ssa->bit_values);
               }
               if(best.count(res_nid))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---updated bitstring: " + bitstring_to_string(best.at(res_nid)));
               }

               for(const auto& def_edge : pn->CGetDefEdgesList())
               {
                  set_value(GET_INDEX_CONST_NODE(def_edge.first));
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzed phi: " + STR(phi));
         }
      }

      for(const auto& stmt : B->CGetStmtList())
      {
         // ga->op1 is equal to var_decl when it binds a newly declared variable to an ssa variable. ie. int a;
         // we can skip this assignment and focus on the ssa variable
         const auto stmt_node = GET_CONST_NODE(stmt);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "Analyzing " + stmt_node->get_kind_text() + "(" + STR(GET_INDEX_CONST_NODE(stmt)) +
                            "): " + STR(stmt_node));
         if(stmt_node->get_kind() == gimple_assign_K)
         {
            const auto ga = GetPointerS<const gimple_assign>(stmt_node);
            THROW_ASSERT(!ga->clobber, "");

            const auto lhs = GET_NODE(ga->op0);
            // handle lhs
            if(lhs->get_kind() == ssa_name_K)
            {
               auto lhs_ssa = GetPointerS<ssa_name>(lhs);
               const auto lhs_nid = GET_INDEX_CONST_NODE(ga->op0);
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->LHS: " + STR(lhs_ssa));
               if(!IsHandledByBitvalue(lhs))
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---variable " + STR(lhs_ssa) + " of type " + STR(tree_helper::CGetType(lhs)) +
                                     " not considered");
               }
               else
               {
                  if(bb_version == 0 || bb_version != function_behavior->GetBBVersion())
                  {
                     lhs_ssa->bit_values.clear();
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---bit_values cleared : " + STR(lhs_ssa));
                  }
                  const auto lhs_signed = tree_helper::IsSignedIntegerType(lhs);
                  if(lhs_signed)
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
                     signed_var.insert(lhs_nid);
                  }

                  const auto ga_op1_kind = GET_CONST_NODE(ga->op1)->get_kind();
                  if(lhs_ssa->CGetUseStmts().empty())
                  {
                     best[lhs_nid] = create_x_bitstring(1);
                  }
                  /// check if this assignment is a load from a constant array
                  else if(ga_op1_kind == array_ref_K || ga_op1_kind == mem_ref_K || ga_op1_kind == target_mem_ref_K ||
                          ga_op1_kind == target_mem_ref461_K || ga_op1_kind == var_decl_K)
                  {
                     // this computation was moved above, to make sure that no
                     // bitstrings (computed by previous execution of the same
                     // bitvalue analysis step) are cleared from ssa variables
                     // before computation of bitvalues for ssa read from ROMs
                     THROW_ASSERT(best.find(lhs_nid) != best.end(), "");
                  }
                  /*if(tree_helper::is_a_pointer(TM, lhs_nid))
                  {
                     HLS_manager*  hm = GetPointer<HLS_manager>(AppM);
                     unsigned int var = tree_helper::get_base_index(TM, lhs_nid);
                     if(var && hm && hm->Rmem && hm->Rmem->get_enable_hls_bit_value() &&
                  function_behavior->is_variable_mem(var)) best[lhs_nid] = create_u_bitstring(pointer_resizing(AppM,
                  lhs_nid, function_behavior, function_id, not_frontend, parameters)); else best[lhs_nid] =
                  create_u_bitstring (tree_helper::TypeSize(GET_NODE(ga->op0)));
                  }*/
                  else if(ga_op1_kind == call_expr_K || ga_op1_kind == aggr_init_expr_K)
                  {
                     const auto ce = GetPointerS<const call_expr>(GET_CONST_NODE(ga->op1));
                     if(GET_CONST_NODE(ce->fn)->get_kind() == addr_expr_K)
                     {
                        const auto addr_node = GET_CONST_NODE(ce->fn);
                        const auto ae = GetPointerS<const addr_expr>(addr_node);
                        const auto fu_decl_node = GET_CONST_NODE(ae->op);
                        THROW_ASSERT(fu_decl_node->get_kind() == function_decl_K, "node  " + STR(fu_decl_node) +
                                                                                      " is not function_decl but " +
                                                                                      fu_decl_node->get_kind_text());
                        const auto ret_type_node = tree_helper::GetFunctionReturnType(fu_decl_node);
                        if(IsHandledByBitvalue(ret_type_node))
                        {
                           const auto called_fd = GetPointer<const function_decl>(fu_decl_node);
                           const auto new_bitvalue =
                               called_fd->bit_values.empty() ?
                                   (called_fd->range ?
                                        called_fd->range->getBitValues(
                                            tree_helper::IsSignedIntegerType(ret_type_node)) :
                                        create_u_bitstring(tree_helper::TypeSize(GET_CONST_NODE(ga->op0)))) :
                                   string_to_bitstring(called_fd->bit_values);
                           if(best[lhs_nid].empty())
                           {
                              best[lhs_nid] = new_bitvalue;
                           }
                           else
                           {
                              best[lhs_nid] = sup(new_bitvalue, best[lhs_nid], lhs);
                           }
                        }
                     }
                     else if(GET_CONST_NODE(ce->fn)->get_kind() != ssa_name_K)
                     {
                        THROW_UNREACHABLE("call node  " + STR(GET_CONST_NODE(ce->fn)) + " is a " +
                                          GET_CONST_NODE(ce->fn)->get_kind_text());
                     }
                  }
                  else if(ga_op1_kind == lut_expr_K)
                  {
                     best[lhs_nid] = create_u_bitstring(1);
                  }
                  else if(ga_op1_kind == extract_bit_expr_K)
                  {
                     best[lhs_nid] = create_u_bitstring(1);
                  }
                  else
                  {
                     if(lhs_ssa->bit_values.empty())
                     {
                        auto u_string = create_u_bitstring(tree_helper::TypeSize(lhs));
                        if(lhs_signed && tree_helper::is_natural(TM, GET_INDEX_CONST_NODE(ga->op0)))
                        {
                           u_string.pop_front();
                           u_string.push_front(bit_lattice::ZERO);
                        }
                        best[lhs_nid] = u_string;
                     }
                     else
                     {
                        best[lhs_nid] = string_to_bitstring(lhs_ssa->bit_values);
                     }
                  }
                  THROW_ASSERT(best.count(lhs_nid), "");
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                 "---updated bitstring: " + bitstring_to_string(best.at(lhs_nid)));
               }
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "RHS: " + GET_CONST_NODE(ga->op1)->get_kind_text());
               INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
            }
         }
         else if(stmt_node->get_kind() == gimple_asm_K)
         {
            const auto ga = GetPointerS<const gimple_asm>(stmt_node);
            if(ga->out)
            {
               const auto tl = GetPointer<const tree_list>(GET_CONST_NODE(ga->out));
               THROW_ASSERT(tl->valu, "only the first output and so only single output gimple_asm are supported");
               const auto out_node = GET_CONST_NODE(tl->valu);
               auto out_ssa = GetPointer<ssa_name>(out_node);
               if(out_ssa && !out_ssa->CGetUseStmts().empty())
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->out: " + STR(out_ssa));

                  if(!IsHandledByBitvalue(out_node))
                  {
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---variable of type " + STR(tree_helper::CGetType(out_node)) + " not considered");
                  }
                  else
                  {
                     out_ssa->bit_values.clear();
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---bit_values cleared : " + STR(out_ssa));
                     if(tree_helper::IsSignedIntegerType(out_node))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "---is signed");
                        signed_var.insert(out_node->index);
                     }
                     best[out_node->index] = create_u_bitstring(tree_helper::TypeSize(out_node));
                     INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                                    "---updated bitstring: " + bitstring_to_string(best.at(out_node->index)));
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
               }
            }
         }
         else
         {
            INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                           "---LHS unhandled for statement kind: " + STR(stmt_node->index));
         }

         std::vector<std::tuple<unsigned int, unsigned int>> vars_read;
         tree_helper::get_required_values(vars_read, stmt);
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "-->Requires " + STR(vars_read.size()) + " values");
         for(const auto& var_pair : vars_read)
         {
            const auto ssa_use_node_id = std::get<0>(var_pair);
            set_value(ssa_use_node_id);
         }
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
         INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level,
                        "Analyzed " + stmt_node->get_kind_text() + ": " + STR(stmt_node));
      }
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "<--");
      INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Analyzed BB" + STR(B->number));
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_PEDANTIC, debug_level, "Initialized best with all variables in the function:");
   print_bitstring_map(best);
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Ended internal initialize");
}

void Bit_Value::clear_current()
{
   for(const auto& b : best)
   {
      if(arguments.find(b.first) != arguments.end() || b.first == function_id)
      {
         current[b.first] = b.second;
      }
      else
      {
         const auto position_in_current = current.find(b.first);
         if(position_in_current != current.end())
         {
            current.erase(position_in_current);
         }
      }
   }
}
