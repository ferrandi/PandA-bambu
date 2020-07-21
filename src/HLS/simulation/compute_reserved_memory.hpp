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
 *              Copyright (c) 2018-2020 Politecnico di Milano
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
 * @file compute_reserved_memory.hpp
 * @brief Specification of the functor used to compute size of objects starting from C initialization string
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef COMPUTE_RESERVED_MEMORY_HPP
#define COMPUTE_RESERVED_MEMORY_HPP

/// Superclass include
#include "c_initialization_parser_functor.hpp"

/// STD include
#include <string>

/// utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(tree_manager);
CONSTREF_FORWARD_DECL(tree_node);

/**
 * Functor used to compute size of memory objects
 */
class ComputeReservedMemory : public CInitializationParserFunctor
{
 private:
   /// The tree manager
   const tree_managerConstRef TM;

   /// The tree node to be stored in memory
   const tree_nodeConstRef tn;

   /// The number of elements in the top level
   unsigned int elements_number;

   /// The current level of {}
   unsigned int depth_level;

 public:
   /**
    * Constructor
    * @param TM is the tree manager
    * @param tn is the variable/parameter to be stored in memory
    */
   ComputeReservedMemory(const tree_managerConstRef TM, const tree_nodeConstRef tn);

   /**
    * Return the computed value
    */
   unsigned int GetReservedBytes() const;

   /**
    * Check that all the necessary information was present in the initialization string
    */
   void CheckEnd() override;

   /**
    * Start the initialization of a new aggregated data structure
    */
   void GoDown() override;

   /**
    * Consume an element of an aggregated data structure
    */
   void GoNext() override;

   /**
    * Ends the initialization of the current aggregated  data structure
    */
   void GoUp() override;

   /**
    * Process an element
    * @param content is the string assocated with the string
    */
   void Process(const std::string& content) override;

   /**
    * In this case the function does not activate anything
    */
   void ActivateFileInit(const std::string&) override
   {
   }

   /**
    * do nothing
    */
   void FinalizeFileInit() override
   {
   }
};
#endif
