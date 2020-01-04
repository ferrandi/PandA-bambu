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
 * @file omp_functions.cpp
 * @brief Datastructure to describe functions allocation in high-level synthesis
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */

/// Header include
#include "omp_functions.hpp"

/// behavior include
#include "function_behavior.hpp"

/// HLS include
#include "hls_manager.hpp"

/// tree include
#include "behavioral_helper.hpp"

/// utility include
#include "dbgPrintHelper.hpp"

OmpFunctions::OmpFunctions(const HLS_managerConstRef _HLSMgr) : HLSMgr(_HLSMgr), locks_allocation(0)
{
}

std::ostream& operator<<(std::ostream& os, const OmpFunctions* omp_functions)
{
   const auto HLSMgr = omp_functions->HLSMgr;
   os << "Function where locks will be allocated:" << std::endl;
   os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(omp_functions->locks_allocation)->CGetBehavioralHelper()->get_function_name() << std::endl;
   os << std::string(indentation, ' ') << "Functions which wrap omp for loops:" << std::endl;
   for(const auto omp_for_wrapper : omp_functions->omp_for_wrappers)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(omp_for_wrapper)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   os << std::string(indentation, ' ') << "Functions that are kernel:" << std::endl;
   for(const auto kernel_functions : omp_functions->kernel_functions)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(kernel_functions)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   os << std::string(indentation, ' ') << "Functions that are atomic:" << std::endl;
   for(const auto atomic_functions : omp_functions->atomic_functions)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(atomic_functions)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   os << std::string(indentation, ' ') << "Functions that are inside the kernel:" << std::endl;
   for(const auto parallelized_functions : omp_functions->parallelized_functions)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(parallelized_functions)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   os << std::string(indentation, ' ') << "Functions that are top of the parallel:" << std::endl;
   for(const auto hierarchical_functions : omp_functions->hierarchical_functions)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(hierarchical_functions)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   os << std::string(indentation, ' ') << "Functions which have to be created in multiple parallel instances:" << std::endl;
   for(const auto parallelized_function : omp_functions->parallelized_functions)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(parallelized_function)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   os << std::string(indentation, ' ') << "Functions which lock/unlock mutexes:" << std::endl;
   for(const auto locking_function : omp_functions->locking_functions)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(locking_function)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   os << std::string(indentation, ' ') << "Functions which propagate accesses to mutexes (in mutual exclusions):" << std::endl;
   for(const auto locks_merge_communication : omp_functions->locks_merge_communication)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(locks_merge_communication)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   os << std::string(indentation, ' ') << "Functions which propagate accesses to mutexes (in parallel):" << std::endl;
   for(const auto locks_parallel_communication : omp_functions->locks_parallel_comunication)
   {
      os << std::string(indentation + 2, ' ') << HLSMgr->CGetFunctionBehavior(locks_parallel_communication)->CGetBehavioralHelper()->get_function_name() << std::endl;
   }
   return os;
}
