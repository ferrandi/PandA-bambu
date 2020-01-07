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
 *              Copyright (c) 2015-2020 Politecnico di Milano
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
 * @file parallel_memory_conn_binding.hpp
 * @brief Data structure used to store the interconnection binding of datapath elements when parallel memory controller is adopted
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef PARALLEL_MEMORY_CONN_BINDING_HPP
#define PARALLEL_MEMORY_CONN_BINDING_HPP

/// Superclass include
#include "conn_binding.hpp"

class ParallelMemoryConnBinding : public conn_binding
{
 public:
   /**
    * Constructor.
    */
   ParallelMemoryConnBinding(const BehavioralHelperConstRef BH, const ParameterConstRef parameters);

   /**
    * Destructor.
    */
   virtual ~ParallelMemoryConnBinding();

   /**
    * Add the interconnection to the structural representation of the datapath
    */
   void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, const structural_managerRef SM);
};
#endif
