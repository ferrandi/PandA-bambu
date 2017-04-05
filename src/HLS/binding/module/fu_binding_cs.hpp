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
 *              Copyright (c) 2004-2016 Politecnico di Milano
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
 * @file fu_binding_cs.hpp
 * @brief Derived class to add module scheduler, mem_ctrl_parallel and bind correctly the channels
 *
 * @author Nicola Saporetti <nicola.saporetti@gmail.com>
*/
#ifndef FU_BINDING_CS_H
#define FU_BINDING_CS_H

#include "fu_binding.hpp"

class fu_binding_cs : public fu_binding
{
   public:

   fu_binding_cs(const HLS_managerConstRef HLS_mgr, const unsigned int function_id, const ParameterConstRef parameters);

   /**
    * @brief manage_memory_ports_parallel_chained if function is hierarchical all first channels must be bind toghether , second toghether and so on
    * @param SM
    * @param memory_modules
    * @param circuit
    * @param HLS
    * @param unique_id
    */
   void manage_memory_ports_parallel_chained(const structural_managerRef SM, const std::set<structural_objectRef> &memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int & unique_id);

   /**
    * Instance the functional unit inside the structural representation of the datapath
    */
   virtual void add_to_SM(const HLS_managerRef HLSMgr, const hlsRef HLS, structural_objectRef clock_port, structural_objectRef reset_port);

   protected:

   /**
    * @brief join_merge_split_cs exit from memory of data of kernel must go to scheduler, datapath_para on mem_ctrl_parallel
    * @param SM
    * @param HLS
    * @param primary_outs
    * @param circuit
    * @param unique_id
    */
   void join_merge_split_cs(const structural_managerRef SM, const hlsRef HLS, std::map<structural_objectRef, std::set<structural_objectRef> > &primary_outs, const structural_objectRef circuit, unsigned int & unique_id);

   /**
    * @brief call_version_of_jms in kernel and parallel use new join merge split
    * @param SM
    * @param HLS
    * @param primary_outs
    * @param circuit
    * @param _unique_id
    */
   void call_version_of_jms(const structural_managerRef SM, const hlsRef HLS, std::map<structural_objectRef, std::set<structural_objectRef> > &primary_outs, const structural_objectRef circuit, unsigned int &_unique_id);

   void manage_memory_ports_parallel_chained_hierarchical(const structural_managerRef SM, const std::set<structural_objectRef> &memory_modules, const structural_objectRef circuit, const hlsRef HLS, unsigned int & _unique_id);

   void connectSplitsToDatapath(std::map<structural_objectRef, std::set<structural_objectRef> >::const_iterator po, const structural_objectRef circuit, const structural_managerRef SM, std::string bus_merger_inst_name, structural_objectRef ss_out_port);
};

#endif // FU_BINDING_CS_H
