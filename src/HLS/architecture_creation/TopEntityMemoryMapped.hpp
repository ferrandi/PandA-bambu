/*
 *                 _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *                _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *               _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *              _/      _/    _/ _/    _/ _/   _/ _/    _/
 *             _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *           ***********************************************
 *                            PandA Project
 *                   URL: http://panda.dei.polimi.it
 *                     Politecnico di Milano - DEIB
 *                      System Architectures Group
 *           ***********************************************
 *            Copyright (C) 2004-2020 Politecnico di Milano
 *
 * This file is part of the PandA framework.
 *
 * The PandA framework is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @file
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @brief Construction of top entity memory mapped interface.
 */

#ifndef _TOPENTITYMEMORYMAPPED_H_
#define _TOPENTITYMEMORYMAPPED_H_

#include "top_entity.hpp"

#include "custom_set.hpp"
#include <list>
#include <string>
class module;

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
   /**
    * Constructor.
    * @param Param The set of parameters.
    * @param HLSMgr The HLS manager.
    * @param funId The tree index of the synthesized function.
    */
   TopEntityMemoryMapped(const ParameterConstRef Param, const HLS_managerRef HLSMgr, unsigned int funId, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor.
    */
   ~TopEntityMemoryMapped() override;

   /**
    * Execute the step
    * @return the exit status of this step
    */
   DesignFlowStep_Status InternalExec() override;

 private:
   /**
    * Allocates the in/out parameters of the module as internal registers
    */
   void allocate_parameters() const;

   void insertMemoryMappedRegister(structural_managerRef SM, structural_objectRef wrappedObj);

   void insertStartDoneLogic(structural_managerRef SM, structural_objectRef wrapperObj);

   void insertStatusRegister(structural_managerRef SM, structural_objectRef wrappedObj);

   void forwardPorts(structural_managerRef SM, structural_objectRef wrappedObj);

   std::list<std::string> ParametersName;
   std::list<structural_objectRef> AddedComponents;
   bool needMemoryMappedRegisters{false};

   /// true when the module is a root function
   bool is_root_function{false};

   void resizing_IO(module* fu_module, unsigned int max_n_ports) const;

   void Initialize() override;
};

#endif /* _TOPENTITYMEMORYMAPPED_H_ */
