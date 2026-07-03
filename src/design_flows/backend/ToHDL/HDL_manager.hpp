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
 * @file HDL_manager.hpp
 * @brief This class writes different HDL based descriptions (VHDL, Verilog, SystemC) starting from a structural
 * representation.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 *
 */
#ifndef HDL_MANAGER_HPP
#define HDL_MANAGER_HPP

#include "HDL_output_mode.hpp"
#include "design_flow_step.hpp"
#include "refcount.hpp"

#include <list>
#include <string>

REF_FORWARD_DECL(HDL_manager);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(language_writer);
REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(structural_manager);
CONSTREF_FORWARD_DECL(technology_manager);
CONSTREF_FORWARD_DECL(Parameter);
enum class HDLWriter_Language;

class HDL_manager
{
 private:
   /// The high level synthesis manager
   const HLS_managerRef HLSMgr;

   /// reference to the target device
   const generic_deviceRef device;

   /// reference to the class containing all the technology information
   const technology_managerConstRef TM;

   /// The structural manager containing top
   const structural_managerRef SM;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   const int debug_level;

   /**
    * Returns the list of components that have a structural-based description.
    * This list of components is relative sorted such that if a component C_i uses a component C_j then C_j is before
    * C_i. To obtain this list the hierarchy is visited in a post-order fashion.
    * @param cir is the structural object under analysis.
    * @param list_of_com is the list of components.
    */
   void get_post_order_structural_components(const structural_objectRef cir,
                                             std::list<structural_objectRef>& list_of_com) const;

   /**
    * Generates the HDL description for the given components in the specified language
    */
   std::string write_components(const std::string& filename, const HDLWriter_Language language,
                                const std::list<structural_objectRef>& components, std::list<std::string>& aux_files,
                                bool is_library = true) const;

   /**
    * Determines the proper language for each component and generates the corresponding HDL descriptions
    */
   void write_components(const std::string& filename, const std::list<structural_objectRef>& components,
                         std::list<std::string>& hdl_files, std::list<std::string>& aux_files,
                         HDL_output_mode gen_mode);

   /**
    * Writes the module description.
    * @param writer is the chosen language writer object.
    * @param cir is the module to be written. The analysis does not consider the inner objects but just one level of the
    * hierarchy.
    * @param is_library true for PANDA/BAMBU IP library components, false for components derived from user input
    */
   void write_module(const language_writerRef writer, const structural_objectRef cir, bool is_library = true) const;

   /**
    * Writes signal port connection post fix.
    * @param writer is the chosen language writer object.
    * @param po is the primary port.
    * @param lspf is true when the first post is written
    */
   void io_signal_fix_ith(const language_writerRef writer, const structural_objectRef po, bool& lspf) const;
   void io_signal_fix_ith_vector(const language_writerRef writer, const structural_objectRef po, bool& lspf) const;

   /**
    * Returns true if the module has a FSM description associated with, false otherwise.
    * @param cir is the module.
    */
   bool is_fsm(const structural_objectRef& cir) const;

   /**
    * Writes a mealy/moore finite state machine behavioral description.
    * @param writer is the chosen language writer object.
    * @param cir is the module.
    * @param fsm_desc is the string-based FSM description.
    * @param fsm_stage_i is the optional stage signal associated with the FSM description.
    */
   void write_fsm(const language_writerRef writer, const structural_objectRef& cir, const std::string& fsm_desc,
                  const std::string& fsm_stage_i) const;

   /**
    * Writes the behavioral description associated with the component
    * @param writer is the chosen language writer object.
    * @param cir is the module.
    * @param behav is the string-based behavioral description.
    */
   void write_behavioral(const language_writerRef writer, const structural_objectRef& cir,
                         const std::string& behav) const;

 public:
   /**
    * Constructor
    * @param _HLSMgr is the high level synthesis manager
    * @param device is the data structure containing information about the target device
    * @param parameters is the data structure containing all the parameters
    */
   HDL_manager(const HLS_managerRef _HLSMgr, const generic_deviceRef device, const ParameterConstRef parameters);

   /**
    * Constructor
    * @param HLSMgr is the high level synthesis manager
    * @param device is the data structure containing information about the target device
    * @param SM is the structural manager containing the top component
    * @param parameters is the data structure containing all the parameters
    */
   HDL_manager(const HLS_managerRef HLSMgr, const generic_deviceRef device, const structural_managerRef SM,
               const ParameterConstRef parameters);

   /**
    * Generates HDL code.
    * @param filename is the base name to be created
    * @param cirs are the structural objects representing the components to be generated
    * @param hdl_files is the list collecting the generated HDL files
    * @param aux_files is the list collecting the generated auxiliary files
    * @param gen_mode preferred HDL output mode
    */
   void hdl_gen(const std::string& filename, const std::list<structural_objectRef>& cirs,
                std::list<std::string>& hdl_files, std::list<std::string>& aux_files,
                HDL_output_mode gen_mode = HDL_OUT_MIX);

   /**
    * Converts a generic string to a language compliant identifier
    */
   static std::string convert_to_identifier(const std::string& id);

   /**
    * Returns the module typename
    * @param cir is the module.
    */
   static std::string get_mod_typename(const structural_objectRef& cir);
};
/// refcount definition of the class
using HDL_managerRef = refcount<HDL_manager>;
#endif
