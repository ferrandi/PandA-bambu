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
 * @file reg_binding.hpp
 * @brief Data structure used to store the register binding of variables
 *
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef REG_BINDING_HPP
#define REG_BINDING_HPP

#include "HLS/fsm/FSMInfo.hpp"
#include "Variable.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"

#include <iosfwd>
#include <map>
#include <string>

REF_FORWARD_DECL(hls);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(reg_binding);
REF_FORWARD_DECL(generic_obj);
REF_FORWARD_DECL(structural_object);
CONSTREF_FORWARD_DECL(FunctionBehavior);
class register_obj;
class reg_binsign_creator;

/**
 * Class managing the register binding.
 * Store the register binding, that is, the mapping of operations in the behavioral description into the set of
 * selected register elements.
 */
class reg_binding : public variable2obj<generic_objRef>
{
   friend class reg_binding_creator;

 public:
   using type_t = enum { STG = 0, CDFG };

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
   std::map<unsigned int, unsigned long long> bitsize_map;

   /// HLS data-structure
   hlsRef HLS;

   /// information about all the HLS synthesis
   const HLS_managerRef HLSMgr;

   /// map between the register and the associated storage value
   std::map<unsigned int, CustomOrderedSet<unsigned int>> reg2storage_values;

   /// set of states where context switch can occur: true for internal cs states, false for inherited cs states
   CustomMap<FSMInfo::state_descriptor, bool> context_switch_states;

   /// set of registers storing variables subject to context switch
   CustomSet<unsigned int> context_switch_regs;

   /// store the set of register without enable
   CustomOrderedSet<unsigned int> is_without_enable;

   /// when true all registers do not require write enable: pipelining comes for free
   bool all_regs_without_enable;

   const FunctionBehaviorConstRef FB;

   const std::string reg_typename;

   /**
    * compute the is with out enable relation
    */
   void compute_is_without_enable();

   /**
    * compute context switch registers out of the context switch states set
    */
   void compute_context_switch_registers();

   /**
    * Specialise a register according to the type of the variables crossing it.
    * @param reg is the register
    * @param r is the id of the register
    */
   virtual void specialise_reg(structural_objectRef& reg, unsigned int r);

 public:
   /**
    * Constructor.
    */
   reg_binding(const hlsRef& HLS, const HLS_managerRef HLSMgr_);

   static reg_bindingRef create_reg_binding(const hlsRef& HLS, const HLS_managerRef HLSMgr_);

   /**
    *
    */
   void bind(unsigned int sv, unsigned int index);

   /**
    * return the name of register to be used
    * @param i is the id of the register
    * @return std::string The FU name for the given register
    */
   virtual std::string GetRegisterFUName(unsigned int i);

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
   void print_el(const_iterator& it) const override;

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
   unsigned long long get_bitsize(unsigned int r) const;

 private:
   /**
    * Returns the set of variable associated with the register
    * @param r is the register
    * @return the set of associated variables
    */
   CustomOrderedSet<std::pair<unsigned int, unsigned int>> get_vars(const unsigned int& r) const;

   /**
    * return and set the bitsize associated with given register
    * @param r is the register
    * @return the bitsize of register r
    */
   unsigned long long compute_bitsize(unsigned int r);
};

/**
 * RefCount type definition of the reg_binding class structure
 */
using reg_bindingRef = refcount<reg_binding>;

#endif
