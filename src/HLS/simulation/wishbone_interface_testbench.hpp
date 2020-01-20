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
 * @file wishbone_interface_tesbench.hpp
 * @brief Class to compute testbenches for high-level synthesis
 *
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

/// Superclass include
#include "testbench_generation_base_step.hpp"

class WishboneInterfaceTestbench : public TestbenchGenerationBaseStep
{
 protected:
   void write_wishbone_input_signal_declaration(const tree_managerConstRef TreeM) const;

   void write_wishbone_callFSM_signal_declaration() const;

   void write_call(bool hasMultiIrq) const override;

   void write_memory_handler() const override;

   void write_interface_handler() const override
   {
   }

   void write_signals(const tree_managerConstRef TreeM, bool& withMemory, bool& hasMultiIrq) const override;

   void write_slave_initializations(bool withMemory) const override;

   void init_extra_signals(bool withMemory) const override;

   void write_file_reading_operations() const override;

   void write_wishbone_output_signal_declaration(bool& withMemory, bool& hasMultiIrq) const;

 public:
   WishboneInterfaceTestbench(const ParameterConstRef _Param, const HLS_managerRef _AppM, const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~WishboneInterfaceTestbench() override;
};
