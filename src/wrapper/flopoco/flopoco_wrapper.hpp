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
 * @file flopoco_wrapper.hpp
 * @brief Wrapper to FloPoCo for VHDL code generation.
 *
 * A object used to invoke the FloPoCo framework
 * to generate VHDL code for floating-point functional units
 *
 * @author Daniele Mastrandrea <daniele.mastrandrea@mail.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef FLOPOCOWRAPPER_HPP
#define FLOPOCOWRAPPER_HPP

/// Autoheader include
#include "config_HAVE_STDCXX_0X.hpp"
#include "config_HAVE_STDCXX_11.hpp"

#include "custom_map.hpp"
#include "custom_set.hpp"
#include <string>
#include <vector>

#include "dbgPrintHelper.hpp"
#include "exceptions.hpp"
#include "refcount.hpp"
#include "simple_indent.hpp"
#include "utility.hpp"

/// Default extension for generated files
#define FILE_EXT ".vhdl"
/// Name of the stored Functional Unit
#define ENCODE_NAME(FU_name, FU_prec_in, FU_prec_out, pipe_parameter) FU_name + "_" + STR(FU_prec_in) + "_" + STR(FU_prec_out) + (pipe_parameter != "" ? "_" + pipe_parameter : "")
/// Additional bits in FloPoCo encoding with reference to IEEE-754 standard
#define FLOPOCO_ADDITIONAL_BITS 2
/// Prefix for the wrapper to the inputs
#define IN_WRAP_PREFIX "in_wrap_"
/// Prefix for the wrapper to the inputs
#define OUT_WRAP_PREFIX "out_wrap_"
/// Suffix appended to the internal (wrapped) Functional Unit
#define WRAPPED_PREFIX "wrapped_"

/**
 * @name forward declarations
 */
//@{
/// RefCount type definition of the flopoco_wrapper class structure
REF_FORWARD_DECL(flopoco_wrapper);
/// Forward declarations of FloPoCo classes
namespace flopoco
{
   class Operator;
   class Target;
} // namespace flopoco
//@}

/**
 * @class flopoco_wrapper
 * Main class for wrapping the FloPoCo code generator.
 */
class flopoco_wrapper
{
 private:
#ifndef NDEBUG
   /// Current debug level
   int debug_level;
#endif
   /// Generated Functional Units
   CustomUnorderedMap<std::string, flopoco::Operator*> FUs;
   /// Set of Functional Units written to a .vhdl file
   CustomUnorderedSet<std::string> FU_files;
   /// Maps a Functional Unit to its precision
   CustomUnorderedMap<std::string, std::pair<unsigned int, unsigned int>> FU_to_prec;
   /// Pretty print functor object used to indent the generated code
   simple_indent PP;
   /// Port types
   typedef enum
   {
      port_in,
      port_out,
      clk,
      rst
   } port_type;
   /// Component types
   typedef enum
   {
      top,
      wrapped,
      in_wrap,
      out_wrap
   } component_type;
   /// unit type
   typedef enum
   {
      UT_ADD,
      UT_SUB,
      UT_MULT,
      UT_DIV,
      UT_FF_CONV,
      UT_ADDSUB,
      UT_UFIX2FP,
      UT_IFIX2FP,
      UT_FP2UFIX,
      UT_FP2IFIX,
      UT_EXP,
      UT_SQRT,
      UT_compare_expr,
      UT_LOG,
      UT_POW,
      UT_UNKNOWN
   } unit_type;

   unit_type type;

   bool signed_p;

   std::vector<flopoco::Operator*> oplist;

   flopoco::Target* target;

   /**
    * Returns one of the generated Functional Units
    * @param FU_name_stored is a string representing the stored FU name
    */
   flopoco::Operator* get_FU(std::string FU_name_stored) const;

   /**
    * Returns the names of ports, according to the needed port type (port_in or port_out)
    * @param FU_name_stored is a string representing the stored FU name
    * @param FU_prec is a number representing the FU precision
    * @param expected_ports is the number of expected ports to be returned
    * @param type is the type of the required ports
    */
   const std::vector<std::string> get_ports(const std::string& FU_name_stored, unsigned int expected_ports, port_type type, bool check_ports = true) const;

   /**
    * Returns the name of a port, according to the needed port type (clock or reset)
    * @param type is the type of the required port
    */
   const std::string get_port(port_type type) const;

   /**
    * Writes the VHDL for a Functional Unit to the desired file name
    * Returns -1 if some problem occurred, 0 if the file was successfully written, 1 if the FU was already written to a file
    * @param FU_name is the Functional Unit whose code must be output
    * @param FU_prec_in_in is a number representing the FU input precision
    * @param FU_prec_out is a number representing the FU output precision
    * @param FU_file is the name of the file, without extension, where the VHDL code should be put (i.e. "FPAdder", not "FPAdder.vhdl")
    * @param pipe_parameter is a string defining the design frequency, in case is not empty
    */
   int InternalWriteVHDL(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, const std::string& FU_file, const std::string& pipe_parameter);

   /**
    * Helper methods for automatic VHDL code generation:
    * @param FU_prefix is a string representing the prefix to prepend to FU_name, when needed
    * @param FU_name_stored is a string representing the stored FU name
    * @param os is the stream where the VHDL code should be put
    * @param type is the type of component under examination
    */
   void outputHeaderVHDL(const std::string& FU_name_stored, std::ostream& os) const;
   void outputWrapVHDL(const std::string& FU_name_stored, std::ostream& os, const std::string& pipe_parameter);
   void outputPortDeclaration(const std::string& FU_prefix, const std::string& FU_name_stored, std::ostream& os, component_type type, const std::string& pipe_parameter);
   void outputSignals(const std::string& FU_name_stored, std::ostream& os);
   void outputPortMap(const std::string& FU_name_stored, std::ostream& os, const std::string& pipe_parameter);

 public:
   /**
    * Constructor
    * @param debug is the current debug level
    */
   flopoco_wrapper(int _debug_level, const std::string& FU_target);

   /**
    * Destructor
    */
   ~flopoco_wrapper();

   // no copy constructor
   flopoco_wrapper(const flopoco_wrapper& inst) = delete;

   /**
    * Adds a Functional Unit to the wrapper
    * @param FU_type is a string representing the FU type
    * @param FU_prec_in is a number representing the FU input precision
    * @param FU_prec_out is a number representing the FU output precision
    * @param FU_name is a string representing the FU name
    * @param pipe_parameter is a string defining the design frequency, in case is not empty
    */
   void add_FU(const std::string& FU_type, unsigned int FU_prec_in, unsigned int FU_prec_out, const std::string& FU_name, const std::string& pipe_parameter);

   /**
    * Returns the Functional Unit's Pipeline Depth
    * @param FU_name is the Functional Unit whose Pipeline Depth is returned
    * @param FU_prec_in is a number representing the FU input precision
    * @param FU_prec_out is a number representing the FU output precision
    */
   unsigned int get_FUPipelineDepth(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, const std::string& pipe_parameter) const;

   /**
    * Writes the VHDL for a Functional Unit to the default file name, which is "FU_name.vhdl"
    * Returns -1 if some problem occurred, 0 if the file was successfully written, 1 if the FU was already written to a file
    * @param FU_name is the Functional Unit whose code must be output
    * @param FU_prec_in_in is a number representing the FU input precision
    * @param FU_prec_out is a number representing the FU output precision
    * @param pipe_parameter is a string defining the design frequency, in case is not empty
    * @param filename is where the name of the produced file will be stored
    */
   int writeVHDL(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, std::string pipe_parameter, std::string& filename);

   /**
    * write the common components
    * @return the name of the generated
    */
   std::string writeVHDLcommon();

   /**
    * Returns the Functional Units that have been written to a VHDL file
    */
   const CustomUnorderedSet<std::string>& get_files_written()
   {
      return this->FU_files;
   }

   /**
    * Checks if a Functional Unit have been written to a VHDL file
    * @param FU_name is the Functional Unit whose existence must be checked
    */
   bool is_unit_written(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, const std::string& pipe_parameter) const
   {
      return this->FU_files.find(ENCODE_NAME(FU_name, FU_prec_in, FU_prec_out, pipe_parameter) + FILE_EXT) != this->FU_files.end();
   }

   /**
    * Return the names of input and output ports (multiple names, thus a vector of strings)
    * @param FU_name is a string representing the FU name
    * @param expected_ports is the number of expected ports to be returned
    */
   const std::vector<std::string> get_in_ports(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, const unsigned int expected_ports, const std::string& pipe_parameter) const
   {
      return get_ports(ENCODE_NAME(WRAPPED_PREFIX + FU_name, FU_prec_in, FU_prec_out, pipe_parameter), expected_ports, port_in);
   }
   const std::vector<std::string> get_out_ports(const std::string& FU_name, const unsigned int FU_prec_in, const unsigned int FU_prec_out, const unsigned int expected_ports, const std::string& pipe_parameter) const
   {
      return get_ports(ENCODE_NAME(WRAPPED_PREFIX + FU_name, FU_prec_in, FU_prec_out, pipe_parameter), expected_ports, port_out);
   }

   /**
    * Retrieve mantissa and exponent for a given precision
    * @param FU_prec is a number representing the FU precision
    * @param n_mant is the number of bits for the mantissa
    * @param n_exp is the number of bits for the exponent
    */
   static void DECODE_BITS(unsigned int FU_prec, unsigned int& n_mant, unsigned int& n_exp);

#if HAVE_STDCXX_11 || HAVE_STDCXX_0X
   static constexpr double DEFAULT_TARGET_FREQUENCY = 100.0;
#else
   static const double DEFAULT_TARGET_FREQUENCY = 100.0;
#endif
};

/// Refcount definition for the flopoco_wrapper class
typedef refcount<flopoco_wrapper> flopoco_wrapperRef;

#endif /* FLOPOCOWRAPPER_H */
