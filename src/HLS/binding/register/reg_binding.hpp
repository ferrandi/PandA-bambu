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
 *              Copyright (C) 2004-2020 Politecnico di Milano
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
 * @file reg_binding.hpp
 * @brief Data structure used to store the register binding of variables
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef REG_BINDING_HPP
#define REG_BINDING_HPP

#include "custom_map.hpp"
#include <iosfwd>
#include <string>

#include "Variable.hpp"
#include "refcount.hpp"

REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(reg_binding);
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(structural_object);
class register_obj;

/**
 * Class managing the register binding.
 * Store the register binding, that is, the mapping of operations in the behavioral description into the set of
 * selected register elements.
 */
class reg_binding : public variable2obj<generic_objRef>
{
 public:
   typedef enum
   {
      STG = 0,
      CDFG
   } type_t;

 protected:
   /// level of the verbosity during the debugging
   int debug;

   /// number of used register
   unsigned int used_regs;

   /// map between register index and object
   std::map<unsigned int, generic_objRef> unique_table;

   /// bind the storage value with the register instance
   std::map<unsigned int, unsigned int> reverse_map;

   /// relation between registers and their bitsize
   std::map<unsigned int, unsigned int> bitsize_map;

   /// HLS datastructure
   hlsRef HLS;

   /// information about all the HLS synthesis
   const HLS_managerRef HLSMgr;

   /// map between the register and the associated storage value
   std::map<unsigned int, CustomOrderedSet<unsigned int>> reg2storage_values;

   /// store the set of register without enable
   CustomOrderedSet<unsigned int> is_without_enable;

   /// when true all registers do not require write enable: pipelining comes for free
   bool all_regs_without_enable;

   /**
    * compute the is with out enable relation
    */
   void compute_is_without_enable();

   /**
    * Specialise a register according to the type of the variables crossing it.
    * @param reg is the register
    * @param reg is the id of the register
    */
   virtual void specialise_reg(structural_objectRef& reg, unsigned int r);

 public:
   /**
    * Constructor.
    */
   reg_binding(const hlsRef& HLS, const HLS_managerRef HLSMgr_);

   /**
    * Destructor.
    */
   virtual ~reg_binding();

   static reg_bindingRef create_reg_binding(const hlsRef& HLS, const HLS_managerRef HLSMgr_);

   /**
    *
    */
   void bind(unsigned int sv, unsigned int index);

   /**
    * return the name of register to be used
    */
   virtual std::string CalculateRegisterName(unsigned int i);

   /**
    * returns number of used register
    * @return the number of used register
    */
   unsigned int get_used_regs() const
   {
      return used_regs;
   }

   /**
    * sets number of used register
    * @param regs is new number of used register
    */
   void set_used_regs(unsigned int regs)
   {
      used_regs = regs;
   }

   /**
    * return the register index where the storage value is stored
    * @param sv is the storage value
    * @return the index of the register assigned to the storage value.
    */
   unsigned int get_register(unsigned int sv) const
   {
      return reverse_map.find(sv)->second;
   }

   /// return true when all registers are without write enable: pipelining comes for free
   bool is_all_regs_without_enable()
   {
      return all_regs_without_enable;
   }

   /**
    * Function that print the register binding associated with a storage value.
    */
   void print_el(const_iterator& it) const;

   /**
    * Returns reference to register object associated to a given index
    * @param r is the register index
    * @return the associated reference
    */
   generic_objRef get(const unsigned int& r) const
   {
      return unique_table.find(r) != unique_table.end() ? unique_table.find(r)->second : generic_objRef();
   }

   /**
    * redefinition of the [] operator
    */
   const register_obj& operator[](unsigned int v);

   /**
    * Add the resulting registers to the structural description of the datapath
    */
   virtual void add_to_SM(structural_objectRef clock_port, structural_objectRef reset_port);

   /**
    * return bitsize
    */
   unsigned int get_bitsize(unsigned int r) const;

 private:
   /**
    * Returns the set of variable associated with the register
    * @param r is the register
    * @return the set of associated variables
    */
   CustomOrderedSet<unsigned int> get_vars(const unsigned int& r) const;

   /**
    * return and set the bitsize associated with given register
    * @param r is the register
    * @return the bitsize of register r
    */
   unsigned int compute_bitsize(unsigned int r);
};

/**
 * RefCount type definition of the reg_binding class structure
 */
typedef refcount<reg_binding> reg_bindingRef;

#endif
