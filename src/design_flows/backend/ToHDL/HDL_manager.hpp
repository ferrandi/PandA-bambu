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
 * @file HDL_manager.hpp
 * @brief This class writes different HDL based descriptions (VHDL, Verilog, SystemC) starting from a structural representation.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef HDL_MANAGER_HPP
#define HDL_MANAGER_HPP

/// Autoheader include
#include "config_HAVE_FLOPOCO.hpp"

/// Superclass include
#include "design_flow_step.hpp"

#include <list>
#include <ostream>
#include <string>
#include <vector>

#include "refcount.hpp"

/**
 * @name Forward declarations.
 */
//@{
REF_FORWARD_DECL(HDL_manager);
REF_FORWARD_DECL(HLS_manager);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(language_writer);
REF_FORWARD_DECL(target_device);
REF_FORWARD_DECL(flopoco_wrapper);
REF_FORWARD_DECL(structural_manager);
CONSTREF_FORWARD_DECL(technology_manager);
CONSTREF_FORWARD_DECL(Parameter);
enum class HDLWriter_Language;
//@}

class HDL_manager
{
 private:
   /// The high level synthesis manager
   const HLS_managerRef HLSMgr;

   /// reference to the target device
   const target_deviceRef device;

   /// reference to the class containing all the technology information
   const technology_managerConstRef TM;

#if HAVE_FLOPOCO
   /// wrapper to the FloPoCo library
   const flopoco_wrapperRef flopo_wrap;
#endif

   /// The structural manager containing top
   const structural_managerRef SM;

   /// The set of input parameters
   const ParameterConstRef parameters;

   /// The debug level
   const int debug_level;

   /**
    * Returns the list of components that have a structural-based description.
    * This list of components is relative sorted such that if a component C_i uses a component C_j then C_j is before C_i.
    * To obtain this list the hierarchy is visited in a post-order fashion.
    * @param lan is the chosen language writer object.
    * @param cir is the structural object under analysis.
    * @param list_of_com is the list of components.
    */
   void get_post_order_structural_components(const structural_objectRef cir, std::list<structural_objectRef>& list_of_com) const;

   /**
    * Generates the HDL description for the given components in the specified language
    */
   std::string write_components(const std::string& filename, const HDLWriter_Language language, const std::list<structural_objectRef>& components, bool equation, std::list<std::string>& aux_files) const;

   /**
    * Determines the proper language for each component and generates the corresponding HDL descriptions
    */
   void write_components(const std::string& filename, const std::list<structural_objectRef>& components, bool equation, std::list<std::string>& hdl_files, std::list<std::string>& aux_files);

   /**
    * Writes the module description.
    * @param lan is the chosen language writer object.
    * @param cir is the module to be written. The analysis does not consider the inner objects but just one level of the hierarchy.
    */
   void write_module(const language_writerRef writer, const structural_objectRef cir, bool equation, std::list<std::string>& aux_files) const;

   /**
    * Writes the FloPoCo module description to a VHDL file.
    * @param cir is the module to be fixed.
    */
   void write_flopoco_module(const structural_objectRef& cir, std::list<std::string>& aux_files) const;

   /**
    * Writes signal port connection post fix.
    * @param lan is the chosen language writer object.
    * @param po is the primary port.
    * @param lspf is true when the first post is written
    */
   void io_signal_fix_ith(const language_writerRef writer, const structural_objectRef po, bool& lspf) const;

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
    */
   void write_fsm(const language_writerRef writer, const structural_objectRef& cir, const std::string& fsm_desc) const;

   /**
    * Writes the behavioral description associated with the component
    * @param writer is the chosen language writer object.
    * @param cir is the module.
    * @param behav is the string-based behavioral description.
    */
   void write_behavioral(const language_writerRef writer, const structural_objectRef& cir, const std::string& behav) const;

 public:
   /**
    * Constructor
    * @param hls_manager is the high level synthesis manager
    * @param device is the data structure containing information about the target device
    * @param parameters is the data structure containing all the parameters
    */
   HDL_manager(const HLS_managerRef hls_manager, const target_deviceRef device, const ParameterConstRef parameters);

   /**
    * Constructor
    * @param HLS is the high level synthesis manager
    * @param device is the data structure containing information about the target device
    * @param SM is the structural manager containing the top component
    * @param parameters is the data structure containing all the parameters
    */
   HDL_manager(const HLS_managerRef HLSMgr, const target_deviceRef device, const structural_managerRef SM, const ParameterConstRef parameters);

   /**
    * Destructor
    */
   ~HDL_manager();

   /**
    * Generates HDL code.
    * @param file_name is the name to be created
    * @param cirs are the structural objects representing the components to be generated
    * @param equation specifies if the equation-based version of the component has to be generated
    * @param the created files (file_name + other files)
    * @param the created aux files
    */
   void hdl_gen(const std::string& file_name, const std::list<structural_objectRef>& cirs, bool equation, std::list<std::string>& hdl_files, std::list<std::string>& aux_files);

   /**
    * Converts a generic string to a language compliant identifier
    */
   static std::string convert_to_identifier(const language_writer* lan, const std::string& id);

   /**
    * Converts a generic string to a language compliant identifier
    */
   static std::string convert_to_identifier(const std::string& id);

   /**
    * Returns the module typename taking into account even the flopoco customizations
    * @param lan is the chosen language writer object.
    * @param cir is the module.
    */
   static std::string get_mod_typename(const language_writer* lan, const structural_objectRef& cir);

#if HAVE_FLOPOCO
   /**
    * return the flopoco object in case it has been allocated
    */
   const flopoco_wrapperRef get_flopocowrapper() const
   {
      return flopo_wrap;
   }
#endif
};
/// refcount definition of the class
typedef refcount<HDL_manager> HDL_managerRef;
#endif
