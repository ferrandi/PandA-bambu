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
 * @file register_obj.hpp
 * @brief Base class for all register into datapath
 *
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */

#ifndef REGISTER_HPP
#define REGISTER_HPP

#include "generic_obj.hpp"
#include "refcount.hpp"
#include "string_manipulation.hpp"

/**
 * class modeling a register object
 */
class register_obj : public generic_obj
{
   generic_objRef wr_enable;

 private:
   unsigned int register_index;

 public:
   /**
    * This is the constructor of the object class, with a given id
    * @param index is the new value for register entry
    */
   explicit register_obj(const unsigned int index) : generic_obj(REGISTER, std::string("reg_") + STR(index))
   {
      register_index = index;
   }

   /**
    * Gets the write enable object for the given register
    * @return a set of sets where each of them can enable register write (when all conditions contained are
    *        true)
    */
   generic_objRef get_wr_enable() const
   {
      return wr_enable;
   }

   /**
    * Sets the write enable for given register
    */
   void set_wr_enable(const generic_objRef& wr_en)
   {
      wr_enable = wr_en;
   }

   /**
    * Gets the index of the register represented by this object
    * @return the index of the represented register
    */
   unsigned int get_register_index()
   {
      return register_index;
   }
};

/// RefCount definition for register_obj class
using register_objRef = refcount<register_obj>;

#endif
