/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************            
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2004-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
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
