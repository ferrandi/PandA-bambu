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
 *   Copyright (C) 2012-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file TopEntityMemoryMapped.hpp
 * @brief Construction of top entity memory mapped interface.
 *
 * @author Marco Minutoli <mminutoli@gmail.com>
 *
 */

#ifndef _TOPENTITYMEMORYMAPPED_H_
#define _TOPENTITYMEMORYMAPPED_H_

#include "custom_set.hpp"
#include "top_entity.hpp"

#include <list>
#include <string>

class module_o;
enum class MemoryAllocation_ChannelsType;

/**
 * @brief Build a wrapper layer on the top entity implementing the
 * momory mapped interface.
 *
 * This step augment the top entity with the logic necessary to build
 * the memory mapped interface when the top function needs it
 * (i.e. when it will be attached to a wishbone bus).
 *
 * The top entity is augmented adding:
 *   - A memory mapped control register.
 *   - A memery mapped register for each function parameters.
 *   - A memory mapped register storing the function return value.
 *
 * If the bus interface is MINIMAL the top function will not contain
 * the memory mapped registers but it will contain the instances of
 * the additional tops.
 */
class TopEntityMemoryMapped : public top_entity
{
 public:
   TopEntityMemoryMapped(const ParameterConstRef _parameters, const HLS_managerRef HLSMgr, unsigned int funId,
                         const DesignFlowManager& design_flow_manager);

   DesignFlowStep_Status InternalExec() override;

 private:
   std::list<std::string> ParametersName;
   std::list<structural_objectRef> AddedComponents;
   bool needMemoryMappedRegisters{false};

   /// true when the module is a root function
   bool is_root_function{false};

   /// Function scope channels number
   unsigned int _channels_number;

   /// Function scope channels type
   MemoryAllocation_ChannelsType _channels_type;

   /**
    * Allocates the in/out parameters of the module as internal registers
    */
   void allocate_parameters() const;

   void insertMemoryMappedRegister(structural_managerRef SM, structural_objectRef wrappedObj);

   void insertStartDoneLogic(structural_managerRef SM, structural_objectRef wrappedObj);

   void insertStatusRegister(structural_managerRef SM, structural_objectRef wrappedObj);

   void resizing_IO(module_o* fu_module, unsigned int max_n_ports) const;

   void Initialize() override;
};

#endif /* _TOPENTITYMEMORYMAPPED_H_ */
