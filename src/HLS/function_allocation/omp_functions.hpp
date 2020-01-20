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
 * @file omp_functions.hpp
 * @brief Datastructure to describe functions allocation in high-level synthesis
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

#ifndef OMP_FUNCTIONS_HPP
#define OMP_FUNCTIONS_HPP

/// Superclass include
#include "functions.hpp"

/// STD includes
#include <ostream>
#include <string>

/// utility include
#include "custom_set.hpp"
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(HLS_manager);

class OmpFunctions : public functions
{
 protected:
   /// The HLS manager
   const HLS_managerConstRef HLSMgr;

 public:
   /// The function where locks have to be instantiated
   unsigned int locks_allocation;

   /// The set of openmp for wrappers
   CustomSet<unsigned int> omp_for_wrappers;

   /// The set of functions which have to be parallelized
   CustomSet<unsigned int> kernel_functions;

   /// The set of functions which have to be parallelized
   CustomSet<unsigned int> atomic_functions;

   /// The set of functions which have to be parallelized
   CustomSet<unsigned int> parallelized_functions;

   /// The set of functions which have to be parallelized
   CustomSet<unsigned int> hierarchical_functions;

   /// The set of functions which lock/unlock mutexes
   CustomSet<unsigned int> locking_functions;

   /// The set of functions propagating accesses to locks in mutual exclusions
   CustomSet<unsigned int> locks_merge_communication;

   /// The set of functions propagating accesses to locks in parallel
   CustomSet<unsigned int> locks_parallel_comunication;

   /**
    * Constructor
    * @param HLSMgr is the HLS manager
    */
   explicit OmpFunctions(const HLS_managerConstRef HLSMgr);

   /**
    * Friend definition of the << operator.
    * @param os is the output stream
    * @param omp_function is the OmpFunction to be printed
    */
   friend std::ostream& operator<<(std::ostream& os, const OmpFunctions* omp_function);
};
#endif
