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
 *              Copyright (C) 2004-2026 Politecnico di Milano
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 *   This file is part of the PandA framework.
 *
 *   Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
/**
 * @file dataport_obj.hpp
 * @brief Base class for all dataports into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */

#ifndef DATAPORT_OBJ_HPP
#define DATAPORT_OBJ_HPP

#include "generic_obj.hpp"
#include <string>
/**
 * primary ports of datapath.
 */
class dataport_obj : public generic_obj
{
   /// define the parameter name of the object
   std::string parameter;

   /// number of bit
   unsigned int bitsize;

   /// data port signedness
   bool signedP;

 public:
   dataport_obj(const std::string& _name, unsigned int _bitsize, bool _signedP)
       : generic_obj(DATA_PORT, _name), bitsize(_bitsize), signedP(_signedP)
   {
   }

   dataport_obj(const std::string& _name, const std::string& _parameter, unsigned int _bitsize, bool _signedP)
       : generic_obj(DATA_PORT, _name), parameter(_parameter), bitsize(_bitsize), signedP(_signedP)
   {
   }

   /**
    * return the maximum bitsize associated with the component
    */
   unsigned int get_bitsize() const
   {
      return bitsize;
   }

   bool isSigned() const
   {
      return signedP;
   }
};

#endif
