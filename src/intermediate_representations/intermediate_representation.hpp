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
 * @file intermediate_representation.hpp
 * @brief Base class for intermediate representation
 *
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 *
 */
#ifndef INTERMEDIATE_REPRESENTATION_HPP
#define INTERMEDIATE_REPRESENTATION_HPP

/// utility include
#include "refcount.hpp"

CONSTREF_FORWARD_DECL(Parameter);

class IntermediateRepresentation
{
 protected:
   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   int debug_level;

 public:
   /**
    * Constructor
    * @param parameters is the set of input parameters
    */
   explicit IntermediateRepresentation(const ParameterConstRef& parameters);

   /**
    * Destructor
    */
   virtual ~IntermediateRepresentation();

   /**
    * Initialize all the data structure
    */
   virtual void Initialize() = 0;

   /**
    * Clear all the data structure
    */
   virtual void Clear() = 0;
};
#endif
