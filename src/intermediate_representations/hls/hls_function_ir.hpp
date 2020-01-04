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
 *              Copyright (C) 2016-2020 Politecnico di Milano
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
 * @file hls_function_ir.hpp
 * @brief Base class for intermediate representation used by HLS ifunction steps
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef HLS_FUNCTION_IR_HPP
#define HLS_FUNCTION_IR_HPP

/// Superclass include
#include "hls_ir.hpp"

/// utility include
#include "refcount.hpp"

REF_FORWARD_DECL(hls);
CONSTREF_FORWARD_DECL(Parameter);

class HLSFunctionIR : public HLSIR
{
 protected:
   /// The hls of the function
   hlsRef hls;

   /// The index of the function to which this IR is associated
   const unsigned int function_index;

 public:
   /**
    * Constructor
    * @param hls_manager is the HLS manager
    * @param function_index is the index of the function to which this IR is associated
    * @param parameters is the set of input parameters
    */
   HLSFunctionIR(const HLS_managerRef hls_manager, const unsigned int function_index, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~HLSFunctionIR() override;
};
#endif
