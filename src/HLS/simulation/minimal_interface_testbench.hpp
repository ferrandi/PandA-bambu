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
 *              Copyright (C) 2004-2023 Politecnico di Milano
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
 * @file minimal_interface_testbench.cpp
 * @brief Class to compute testbenches for high-level synthesis
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @author Manuel Beniani <manuel.beniani@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Marco Lattuada <marco.lattuada@polimi.it>
 * @author Pietro Fezzardi <pietrofezzardi@gmail.com>
 *
 */

#ifndef MINIMAL_INTERFACE_TESTBENCH_HPP
#define MINIMAL_INTERFACE_TESTBENCH_HPP

/// Superclass include
#include "testbench_generation_base_step.hpp"

class MinimalInterfaceTestbench : public TestbenchGenerationBaseStep
{
 protected:
   std::string memory_aggregate_slices(unsigned int i, unsigned long long bitsize,
                                       unsigned long long Mout_addr_ram_bitsize) const;

   std::string memory_aggregate_slices_queue(unsigned int i, unsigned long long bitsize,
                                             unsigned long long Mout_addr_ram_bitsize,
                                             const std::string& queue_type) const;

   void cond_load(unsigned long long Mout_addr_ram_bitsize, const std::string& post_slice2,
                  const std::string& res_string, unsigned int i, const std::string& in_else,
                  const std::string& mem_aggregate) const;

   void cond_load_from_queue(unsigned long long Mout_addr_ram_bitsize, const std::string& queue_type,
                             const std::string& post_slice2, const std::string& res_string, unsigned int i,
                             const std::string& in_else, const std::string& mem_aggregate) const;

   void write_call(bool hasMultiIrq) const override;

   void update_memory_queue(std::string port_name, std::string delay_type) const;

   void write_memory_handler() const override;

   void write_interface_handler() const override;

   void write_signal_queue(std::string port_name, std::string delay_type) const;

   void write_signals(const tree_managerConstRef TreeM, bool& withMemory, bool& hasMultiIrq) const override;

   void write_slave_initializations(bool with_memory) const override;

   /// specialize read_input_value_from_file for interface PI_RNONE
   void read_input_value_from_file_RNONE(const std::string& input_name, bool& first_valid_input,
                                         unsigned long long bitsize) const;

   void write_read_fifo_manager(std::string par, const std::string& pi_dout_name, unsigned long long bitsize,
                                std::string valid_suffix) const;

   void write_file_reading_operations() const override;

   void write_input_signal_declaration(const tree_managerConstRef TreeM, bool& with_memory) const;

   void write_output_signal_declaration() const;

 public:
   /**
    * Constructor
    */
   MinimalInterfaceTestbench(const ParameterConstRef _Param, const HLS_managerRef _AppM,
                             const DesignFlowManagerConstRef design_flow_manager);

   /**
    * Destructor
    */
   ~MinimalInterfaceTestbench() override;
};
#endif
