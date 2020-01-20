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
 * @file hls_ir.hpp
 * @brief Base class for intermediate representation used by HLS steps
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef HLS_IR_HPP
#define HLS_IR_HPP

/// Superclass include
#include "intermediate_representation.hpp"

/// utility include
#include "refcount.hpp"

REF_FORWARD_DECL(HLS_manager);

class HLSIR : public IntermediateRepresentation
{
 protected:
   /// The HLS manager
   const HLS_managerRef hls_manager;

 public:
   /**
    * Constructor
    * @param hls_manager is the HLS manager
    * @param parameters is the set of input parameters
    */
   HLSIR(const HLS_managerRef& hls_manager, const ParameterConstRef& parameters);

   /**
    * Destructor
    */
   ~HLSIR() override;

   /**
    * Initialize all the data structure
    */
   void Initialize() override = 0;

   /**
    * Clear all the data structure
    */
   void Clear() override = 0;
};
#endif
