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
 * @file technology_node.hpp
 * @brief Class specification of the data structures used to manage technology information.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef TECHNOLOGY_NODE_HPP
#define TECHNOLOGY_NODE_HPP

/// Autoheader include
#include "config_HAVE_BOOLEAN_PARSER_BUILT.hpp"
#include "config_HAVE_CIRCUIT_BUILT.hpp"
#include "config_HAVE_EXPERIMENTAL.hpp"

#include "custom_map.hpp"
#include "refcount.hpp"
#include "simple_indent.hpp"
#include "utility.hpp"
#include <ostream>
#include <string>
#include <vector>

/**
 * @name forward declarations
 */
//@{
/// RefCount type definition of the technology_node class structure
REF_FORWARD_DECL(technology_node);
#if HAVE_CIRCUIT_BUILT
/// RefCount type definition of the structural_manager class structure
REF_FORWARD_DECL(structural_manager);
#endif
/// forward decl of xml Element
class xml_element;
/// forward decl of xml Element
REF_FORWARD_DECL(xml_node);
/// RefCount type definition of the technology_manager class structure
REF_FORWARD_DECL(technology_manager);
CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(attribute);
/// RefCount definition of the resource information of the component
REF_FORWARD_DECL(area_model);
/// RefCount definition of the layout information of the component
REF_FORWARD_DECL(layout_model);
/// RefCount definition of the timing information of the component
REF_FORWARD_DECL(time_model);
/// RefCount definition of the power information of the component
REF_FORWARD_DECL(power_model);
REF_FORWARD_DECL(target_device);
enum class TargetDevice_Type;
//@}

/// FPGA modules
#define LUT_GATE_STD "LUT"
#define IBUF_GATE_STD "IBUF"
#define OBUF_GATE_STD "OBUF"

// Logic Ports
#define AND_GATE_STD "AND_GATE"
#define NAND_GATE_STD "NAND_GATE"
#define OR_GATE_STD "OR_GATE"
#define NOR_GATE_STD "NOR_GATE"
#define XOR_GATE_STD "XOR_GATE"
#define XNOR_GATE_STD "XNOR_GATE"
#define NOT_GATE_STD "NOT_GATE"
#define DFF_GATE_STD "DFF_GATE"
#define BUFF_GATE_STD "BUFF_GATE"

#define TEST_MUL_MUX_8 "TEST_MUL_MUX_8"
#define MUX_GATE_STD "MUX_GATE"
#define DEMUX_GATE_STD "DEMUX_GATE"
#define MUX_N_TO_1 "MUX_N_to_1"
#define MULTIPLIER_STD "mult_expr_FU"
#define UI_MULTIPLIER_STD "ui_mult_expr_FU"
#define UI_CONST_MULTIPLIER_STD "ui_const_mult_expr_FU"
#define ADDER_STD "plus_expr_FU"
#define COND_EXPR_STD "cond_expr_FU"
#define UI_ADDER_STD "ui_plus_expr_FU"
#define UI_ALIGN_ADDER_STD "ui_align_plus_expr_FU"
#define CONCAT4_STD "concat_4_constructor"
#define SIGNED_BITFIELD_FU_STD "Sbitfield_FU"
#define UNSIGNED_BITFIELD_FU_STD "Ubitfield_FU"
#define CONSTANT_STD "constant_value"
#define ASSIGN_SIGNED_STD "ASSIGN_SIGNED_FU"
#define ASSIGN_UNSIGNED_STD "ASSIGN_UNSIGNED_FU"
#define ASSIGN_REAL_STD "ASSIGN_REAL_FU"
#define ASSIGN_VECTOR_BOOL_STD "ASSIGN_VECTOR_BOOL_FU"
#define ASSIGN_VEC_SIGNED_STD "ASSIGN_VEC_SIGNED_FU"
#define ASSIGN_VEC_UNSIGNED_STD "ASSIGN_VEC_UNSIGNED_FU"
#define ADDR_EXPR_STD "addr_expr_FU"
#define ASSERT_EXPR_SIGNED_STD "assert_expr_FU"
#define ASSERT_EXPR_UNSIGNED_STD "ui_assert_expr_FU"
#define ASSERT_EXPR_REAL_STD "fp_assert_expr_FU"
#define BMEMORY_STD "BMEMORY_CTRL"
#define BMEMORY_STDN "BMEMORY_CTRLN"
#define MEMCPY_STD "__internal_bambu_memcpy"
#define ARRAY_1D_STD_BRAM "ARRAY_1D_STD_BRAM"
#define ARRAY_1D_STD_BRAM_SDS "ARRAY_1D_STD_BRAM_SDS"
#define ARRAY_1D_STD_BRAM_SDS_BUS "ARRAY_1D_STD_BRAM_SDS_BUS"
#define ARRAY_1D_STD_DISTRAM_SDS "ARRAY_1D_STD_DISTRAM_SDS"
#define ARRAY_1D_STD_BRAM_N1 "ARRAY_1D_STD_BRAM_N1"
#define ARRAY_1D_STD_BRAM_N1_SDS "ARRAY_1D_STD_BRAM_N1_SDS"
#define ARRAY_1D_STD_DISTRAM_N1_SDS "ARRAY_1D_STD_DISTRAM_N1_SDS"
#define ARRAY_1D_STD_BRAM_N1_SDS_BUS "ARRAY_1D_STD_BRAM_N1_SDS_BUS"
#define ARRAY_1D_STD_BRAM_NN "ARRAY_1D_STD_BRAM_NN"
#define ARRAY_1D_STD_BRAM_NN_SDS "ARRAY_1D_STD_BRAM_NN_SDS"
#define ARRAY_1D_STD_BRAM_NN_SDS_BUS "ARRAY_1D_STD_BRAM_NN_SDS_BUS"
#define ARRAY_1D_STD_DISTRAM_NN_SDS "ARRAY_1D_STD_DISTRAM_NN_SDS"
#define STD_BRAM "STD_BRAM"
#define STD_BRAMN "STD_BRAMN"
#define MEMLOAD_STD "__builtin_memload"
#define MEMSTORE_STD "__builtin_memstore"
#define MEMLOAD_STDN "__builtin_memload_N"
#define MEMSTORE_STDN "__builtin_memstore_N"
#define BUILTIN_EXIT_STD "__builtin_exit"
#define BUILTIN_ABORT_STD "__builtin_abort"
#define BUILTIN_WAIT_CALL_STD "__builtin_wait_call"
#define PROXY_CTRL "PROXY_CTRL"
#define PROXY_CTRLN "PROXY_CTRLN"
#define DPROXY_CTRL "DPROXY_CTRL"
#define DPROXY_CTRLN "DPROXY_CTRLN"

#define MEMORY_TYPE_ASYNCHRONOUS "ASYNCHRONOUS"
#define MEMORY_TYPE_SYNCHRONOUS_UNALIGNED "SYNCHRONOUS_UNALIGNED"
#define MEMORY_TYPE_SYNCHRONOUS_SDS "SYNCHRONOUS_SDS"
#define MEMORY_TYPE_SYNCHRONOUS_SDS_BUS "SYNCHRONOUS_SDS_BUS"

#define CHANNELS_TYPE_MEM_ACC_11 "MEM_ACC_11"
#define CHANNELS_TYPE_MEM_ACC_N1 "MEM_ACC_N1"
#define CHANNELS_TYPE_MEM_ACC_NN "MEM_ACC_NN"
#define CHANNELS_TYPE_MEM_ACC_P1N "MEM_ACC_P1N"
#define CHANNELS_TYPE_MEM_ACC_CS "MEM_ACC_CS"

#define MEMORY_CTRL_TYPE_D00 "D00"
#define MEMORY_CTRL_TYPE_PROXY "PROXY"
#define MEMORY_CTRL_TYPE_DPROXY "DPROXY"
#define MEMORY_CTRL_TYPE_PROXYN "PROXYN"
#define MEMORY_CTRL_TYPE_DPROXYN "DPROXYN"

#define UUDATA_CONVERTER_STD "UUdata_converter_FU"
#define IUDATA_CONVERTER_STD "IUdata_converter_FU"
#define UIDATA_CONVERTER_STD "UIdata_converter_FU"
#define IIDATA_CONVERTER_STD "IIdata_converter_FU"
#define BIVECTOR_CONVERTER_STD "BIvector_converter_FU"
#define BUVECTOR_CONVERTER_STD "BUvector_converter_FU"
#define FFDATA_CONVERTER_STD "FFdata_converter_FU"
#define SF_FFDATA_CONVERTER_32_64_STD "sf_FFdata_converter_FU_32_64"
#define SF_FFDATA_CONVERTER_64_32_STD "sf_FFdata_converter_FU_64_32"

#define UUCONVERTER_EXPR_STD "UUconvert_expr_FU"
#define IUCONVERTER_EXPR_STD "IUconvert_expr_FU"
#define UICONVERTER_EXPR_STD "UIconvert_expr_FU"
#define IICONVERTER_EXPR_STD "IIconvert_expr_FU"
#define VIEW_CONVERT_STD_INT "view_convert_expr_FU"
#define VIEW_CONVERT_STD_UINT "ui_view_convert_expr_FU"
#define VIEW_CONVERT_STD_REAL "fp_view_convert_expr_FU"

#define EXTRACT_BIT_EXPR_SIGNED_STD "extract_bit_expr_FU"
#define EXTRACT_BIT_EXPR_UNSIGNED_STD "ui_extract_bit_expr_FU"
#define LUT_EXPR_STD "lut_expr_FU"

// For distributed controller
#define CE_STD "CE_FU"
#define CE_FSM "CE_FSM"
#define CE_BYPASS "CE_BYPASS"
#define JOIN_STD "JOIN_FU"
#define SIMPLEJOIN_STD "SIMPLEJOIN_FU"
#define PT_STD "PT_FU"
#define START_STD "START_FU"
#define COND_STD "COND_FU"
#define SWITCH_CASE_STD "SWITCH_CASE_FU"
#define MC_STD "MC_FU"
#define FC_STD "FC_FU"

/// simple register without reset
#define register_STD "register_STD"

/// register with synchronous reset
#define register_SR "register_SR"

/// flipflop with synchronous reset
#define flipflop_SR "flipflop_SR"

/// register with asynchronous reset
#define register_AR "register_AR"

/// flipflop with asynchronous reset
#define flipflop_AR "flipflop_AR"

/// register with asynchronous reset no retime
#define register_AR_NORETIME "register_AR_NORETIME"
#define register_AR_NORETIME_INT "register_AR_NORETIME_INT"
#define register_AR_NORETIME_UINT "register_AR_NORETIME_UINT"
#define register_AR_NORETIME_REAL "register_AR_NORETIME_REAL"
/// register with synchronous enable
#define register_SE "register_SE"
/// register with synchronous reset and synchronous enable
#define register_SRSE "register_SRSE"
/// register with asynchronous reset and synchronous enable
#define register_ARSE "register_ARSE"
/// register with synchronized asynchronous reset and synchronous enable
#define register_SARSE "register_SARSE"
/// memory mapped register
#define MEMORY_MAPPED_REGISTER_FU "memory_mapped_register_FU"
#define RETURN_MM_REGISTER_FU "return_value_mm_register_FU"
#define NOTYFY_CALLER_MINIMAL_FU "notify_caller_minimal_FU"
#define STATUS_REGISTER_FU "status_register_FU"
#define STATUS_REGISTER_NO_NOTIFIED_FU "status_register_no_notified_FU"
#define MEMORY_MAPPED_REGISTERN_FU "memory_mapped_registerN_FU"
#define RETURN_MM_REGISTERN_FU "return_value_mm_registerN_FU"
#define NOTYFY_CALLER_MINIMALN_FU "notify_caller_minimalN_FU"
#define STATUS_REGISTERN_FU "status_registerN_FU"
#define STATUS_REGISTER_NO_NOTIFIEDN_FU "status_register_no_notifiedN_FU"

/**
 * Macro which defines the get_kind_text function that returns the parameter as a string.
 */
#define GET_TEC_KIND_TEXT(meth)               \
   std::string get_kind_text() const override \
   {                                          \
      return std::string(#meth);              \
   }

/**
 * Enumerative type for technology object classes, it is used with get_kind() function
 * to know the actual type of a technology_node.
 */
enum tec_kind
{
   operation_K,
   functional_unit_K,
   functional_unit_template_K,
   storage_unit_K
};

/**
 * Macro used to implement get_kind() function in structural_object hierarchy classes
 */
#define GET_TEC_KIND(meth)                 \
   enum tec_kind get_kind() const override \
   {                                       \
      return (meth##_K);                   \
   }

/**
 * Abstract pure class for the technology structure. This node and in particular its refCount type will be used to describe all
 * nodes used to build a technology description.
 */
struct technology_node
{
   /**
    * Constructor
    */
   technology_node();

   /**
    * Destructor
    */
   virtual ~technology_node();

   /**
    * Return the name of the technology node.
    */
   virtual const std::string& get_name() const = 0;

   /**
    * Load a technology_node starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param TM is the technology manager.
    */
   virtual void xload(const xml_element* Enode, const technology_nodeRef owner, const ParameterConstRef Param, const target_deviceRef device) = 0;

   /**
    * Add a technology_node to an xml tree.
    * @param rootnode is the root node at which the xml representation of the technology node is attached.
    */
   virtual void xwrite(xml_element* rootnode, const technology_nodeRef tn, const ParameterConstRef Param, TargetDevice_Type type) = 0;

   /**
    * Append a technology_node to a liberty file.
    */
   virtual void lib_write(std::ofstream& os, const simple_indentRef PP) = 0;

   /**
    * Append a technology_node to a LEF file.
    */
   virtual void lef_write(std::ofstream& os, const simple_indentRef PP) = 0;

   /**
    * Virtual function that prints the class.
    * @param os is the output stream
    */
   virtual void print(std::ostream& os) const = 0;

   /**
    * Virtual function used to get the string name
    * of a technology_node instance.
    * @return a string identifying the object type.
    */
   virtual std::string get_kind_text() const = 0;

   /**
    * Virtual function used to find the real type
    * of a technology_nodeinstance.
    * @return a tec_kind enum identifying the object type.
    */
   virtual enum tec_kind get_kind() const = 0;

   /**
    * Friend definition of the << operator. Pointer version.
    */
   friend std::ostream& operator<<(std::ostream& os, const technology_nodeRef& s)
   {
      if(s)
         s->print(os);
      return os;
   }

 protected:
   /// pretty print functor object used by all print members to indent the output of the print function.
   static simple_indent PP;
};
/// refcount definition of the class
typedef refcount<technology_node> technology_nodeRef;
typedef refcount<const technology_node> technology_nodeConstRef;

/**
 * This class specifies the characteristic of a particular operation working on a given functional unit.
 */
struct operation : public technology_node
{
   /// name of the operation mappen on a given functional unit.
   std::string operation_name;

   /// class representing the timing information associated with this operation
   time_modelRef time_m;

#if HAVE_EXPERIMENTAL
   /// This variable stores the power information of the component with respect to this operation
   power_modelRef power_m;
#endif

   /// property of commutativity
   bool commutative;

   /// flag to determine if the operation is bounded or not
   bool bounded;

   /// true when the primary input of the functional unit are registered
   bool primary_inputs_registered;

   /// supported types and precision of the operation, in form (name, list_of_prec).
   std::map<std::string, std::vector<unsigned int>> supported_types;

   /// comma separated string with the parameter for the different implementation of the pipeline. Empty when the operation is not pipelined
   std::string pipe_parameters;

   /// comma separed string with the parameter for different portsize values.
   std::string portsize_parameters;

   /**
    * Constructor
    */
   operation();

   /**
    * Destructor
    */
   ~operation() override;

   /**
    * Returns the name of the operation.
    */
   const std::string& get_name() const override
   {
      return operation_name;
   }

   /**
    * Checks if the specified type name is supported.
    * If the attribute "supported_types" is not defined, it will always return true.
    * @param type_name is the name of the type
    */
   bool is_type_supported(std::string type_name) const;

   /**
    * Checks if the specified type name is supported
    * If the attribute "supported_types" is not defined, it will always return true.
    * @param type_name is the name of the type
    * @param type_prec is the type precision
    */
   bool is_type_supported(std::string type_name, unsigned int type_prec) const;

   /**
    * Checks if the specified type name is supported with the max precision in type_prec.
    * If the attribute "supported_types" is not defined, it will always return true.
    * @param type_name is the name of the type
    * @param type_prec is the vector of type precisions
    */
   bool is_type_supported(const std::string& type_name, const std::vector<unsigned int>& type_prec, const std::vector<unsigned int>& type_n_element) const;

   /**
    * Returns the supported type as a string
    */
   std::string get_type_supported_string() const;

   /**
    * Load a operation node starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param TM is the technology manager.
    */
   void xload(const xml_element* Enode, const technology_nodeRef owner, const ParameterConstRef Param, const target_deviceRef device) override;

   /**
    * Add a operation node to an xml tree.
    * @param rootnode is the root node at which the xml representation of the operation is attached.
    */
   void xwrite(xml_element* rootnode, const technology_nodeRef tn, const ParameterConstRef Param, const TargetDevice_Type type) override;

   /**
    * Append a technology_node to a liberty file.
    */
   void lib_write(std::ofstream& os, const simple_indentRef PP) override;

   /**
    * Append a technology_node to a LEF file.
    */
   void lef_write(std::ofstream& os, const simple_indentRef PP) override;

   /**
    * Return the datastructure containing the pin-to-pin delay for this operation
    */
   std::map<std::string, std::map<std::string, double>> get_pin_to_pin_delay() const;

   bool is_bounded() const
   {
      return bounded;
   }

   bool is_primary_inputs_registered() const
   {
      return primary_inputs_registered;
   }

   /**
    * function that prints the class operation.
    * @param os is the output stream.
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_TEC_KIND_TEXT(operation)

   /**
    * Redefinition of get_kind()
    */
   GET_TEC_KIND(operation)
};

/**
 * This class specifies the characteristic of a particular functional unit.
 */
struct functional_unit : public technology_node
{
   typedef enum
   {
      UNKNOWN = 0,
      COMBINATIONAL,
      STATETABLE,
      FF,
      LATCH,
      PHYSICAL
   } type_t;

   /// Type definition of a vector of functional_unit.
   typedef std::vector<technology_nodeRef> operation_vec;

   /// return the logical type of the cell
   type_t logical_type;

   /// name of the functional unit.
   std::string functional_unit_name;

   /// list of attributes associated to the components
   std::vector<std::string> ordered_attributes;

   /// map between the attribute keys and the corresponding values
   std::map<std::string, attributeRef> attributes;

   /// clock period adopted for the synthesis (in ns)
   double clock_period;

   /// clock period resource fraction
   double clock_period_resource_fraction;

   /// This variable stores the resource information of the component.
   area_modelRef area_m;

#if HAVE_EXPERIMENTAL
   /// layout information of the component.
   layout_modelRef layout_m;
#endif

#if HAVE_CIRCUIT_BUILT
   /// The current structural representation of the component.
   structural_managerRef CM;
#endif

   /// pointer to the XML description of the cell
   xml_nodeRef XML_description;

   /// Name of the template.
   std::string fu_template_name;

   /// Template parameters.
   std::string fu_template_parameters;

   /// Value used during the characterization of this instance.
   std::string characterizing_constant_value;

   /// Specify the type of memory the functional unit is compliant with.
   /// non-null only in case the functional unit is a memory.
   std::string memory_type;

   /// Specify the type of the channel the functional unit is compliant with.
   std::string channels_type;

   /// Specify the type of memory controller the functional unit is compliant with.
   /// non-null only in case the functional unit is a memory controller.
   std::string memory_ctrl_type;

   /// Specify the bram load latency the functional unit is compliant with.
   /// non-null only in case the functional unit is a memory or a memory controller.
   std::string bram_load_latency;

   /// Specify that the functional unit has the same timing per operation
   /// it has the one specified by this field
   std::string component_timing_alias;

   /// The timestamp of the characterization of this functional unit
   TimeStamp characterization_timestamp;

   /**
    * Constructor
    */
   functional_unit();

   /**
    * Constructor
    */
   explicit functional_unit(const xml_nodeRef XML_description);

   /**
    * Destructor
    */
   ~functional_unit() override;

   /**
    * Add the given operation to the current functional_unit.
    * @param curr is the added element
    */
   void add(const technology_nodeRef& curr)
   {
      if(op_name_to_op.count(curr->get_name()))
      {
         auto del = std::find(list_of_operation.begin(), list_of_operation.end(), op_name_to_op[curr->get_name()]);
         if(del != list_of_operation.end())
            list_of_operation.erase(del);
      }
      list_of_operation.push_back(curr);
      op_name_to_op[curr->get_name()] = curr;
   }

   /**
    * Sets the clock period adopted for the synthesis (0 means that it has been generated with unconstrained synthesis) (in ns)
    */
   void set_clock_period(double _clock_period);

   /**
    * Returns the clock period adopted for the synthesis (0 means that it has been generated with unconstrained synthesis) (in ns)
    */
   double get_clock_period() const
   {
      return clock_period;
   }

   /**
    * Sets the clock period resource fraction adopted for the synthesis
    */
   void set_clock_period_resource_fraction(double _clock_period_resource_fraction);

   /**
    * Returns the clock period resource fraction adopted for the synthesis
    */
   double get_clock_period_resource_fraction() const
   {
      return clock_period_resource_fraction;
   }

   /**
    * Return the operations that the functional unit can handle.
    * @return operations that the functional unit can handle.
    */
   const operation_vec& get_operations() const
   {
      return list_of_operation;
   }

   /**
    * Return the number of the operations that the functional unit can handle.
    * @return an integer representing the number of operations that the functional unit can handle.
    */
   size_t get_operations_num() const
   {
      return list_of_operation.size();
   }

   /**
    * This method returns the operationRef from its name if the
    * functional unit contains an operation of type op_name.
    */
   technology_nodeRef get_operation(const std::string& op_name) const;

   /**
    * Return the name of the operation.
    */
   const std::string& get_name() const override
   {
      return functional_unit_name;
   }

   /**
    * Load a functional unit starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    */
   void xload(const xml_element* node, const technology_nodeRef owner, const ParameterConstRef Param, const target_deviceRef device) override;

#if HAVE_BOOLEAN_PARSER_BUILT
   /**
    * Load a functional unit starting from a genlib string definition.
    * @param definition is a string representing the functional unit
    * @param owner is the refcount version of this.
    */
   void gload(const std::string& definition, const std::string& fu_name, technology_nodeRef owner, const ParameterConstRef Param);
#endif

   /**
    * Add a functional unit to an xml tree.
    * @param rootnode is the root node at which the xml representation of the functional unit is attached.
    */
   void xwrite(xml_element* rootnode, const technology_nodeRef tn, const ParameterConstRef Param, const TargetDevice_Type type) override;

   /**
    * Append a technology_node to a liberty file.
    */
   void lib_write(std::ofstream& os, const simple_indentRef PP) override;

   /**
    * Append a technology_node to a LEF file.
    */
   void lef_write(std::ofstream& os, const simple_indentRef PP) override;

   /**
    * function that prints the class functional_unit.
    * @param os is the output stream.
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_TEC_KIND_TEXT(functional_unit)

   /**
    * Redefinition of get_kind()
    */
   GET_TEC_KIND(functional_unit)

 private:
   /// This varibale maps the name of the op with its reference.
   std::map<std::string, technology_nodeRef> op_name_to_op;

   /// At each functional unit can be associate several operations with different performances.
   operation_vec list_of_operation;

   friend class technology_manager;
};

/**
 * This class describe a functional unit template.
 */
struct functional_unit_template : public technology_node
{
   /// Functional Unit.
   technology_nodeRef FU;

   /// when non empty it defines with respect what the functional unit template has been specialized
   std::string specialized;

   /// Value used during the characterization of templates.
   std::string characterizing_constant_value;

   /// Specify the type of memory the functional unit is compliant with.
   /// non-null only in case the functional unit is a memory.
   std::string memory_type;

   /// Specify the type of the channel the functional unit is compliant with.
   std::string channels_type;

   /// Specify the type of memory controller the functional unit is compliant with.
   /// non-null only in case the functional unit is a memory controller.
   std::string memory_ctrl_type;

   /// Specify the bram load latency the functional unit is compliant with.
   /// non-null only in case the functional unit is a memory or a memory controller.
   std::string bram_load_latency;

   /// The template will not consider constants connected to the inputs during the module characterization
   bool no_constant_characterization;

   /// Constructor
   //@{
   functional_unit_template();

   explicit functional_unit_template(const xml_nodeRef XML_description);
   //@}

   /**
    * Return the name of the operation.
    */
   const std::string& get_name() const override
   {
      return FU->get_name();
   }

   /**
    * Load a functional unit starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    */
   void xload(const xml_element* Enode, const technology_nodeRef owner, const ParameterConstRef Param, const target_deviceRef device) override;

   /**
    * Add a functional unit to an xml tree.
    * @param rootnode is the root node at which the xml representation of the functional unit is attached.
    */
   void xwrite(xml_element* rootnode, const technology_nodeRef tn, const ParameterConstRef Param, TargetDevice_Type type) override;

   /**
    * Append a technology_node to a liberty file.
    */
   void lib_write(std::ofstream& os, const simple_indentRef PP) override;

   /**
    * Append a technology_node to a LEF file.
    */
   void lef_write(std::ofstream& os, const simple_indentRef PP) override;

   /**
    * function that prints the class functional_unit.
    * @param os is the output stream.
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_TEC_KIND_TEXT(functional_unit_template)

   /**
    * Redefinition of get_kind()
    */
   GET_TEC_KIND(functional_unit_template)
};

/**
 * This class specifies the characteristic of a particular storage unit.
 */
struct storage_unit : public technology_node
{
   /// name of the storage_unit.
   std::string storage_unit_name;

   double bits;

   double words;

   unsigned int read_ports;
   double read_latency;

   unsigned int write_ports;
   double write_latency;

   unsigned int readwrite_ports;
   double readwrite_latency;

   double area;

#if HAVE_CIRCUIT_BUILT
   structural_managerRef CM;
#endif

   /// Constructor
   storage_unit() : bits(0), words(0), read_ports(0), read_latency(0), write_ports(0), write_latency(0), readwrite_ports(0), readwrite_latency(0), area(0)
   {
   }

   /// Destructor
   ~storage_unit() override = default;

   const std::string& get_name() const override
   {
      return storage_unit_name;
   }
   /**
    * Load a storage_unit starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param TM is the technology manager.
    */
   void xload(const xml_element*, const technology_nodeRef, const ParameterConstRef, const target_deviceRef) override
   {
      abort();
   }

   /**
    * Add a storage_unit to an xml tree.
    * @param rootnode is the root node at which the xml representation of the storage_unit unit is attached.
    */
   void xwrite(xml_element*, const technology_nodeRef, const ParameterConstRef, TargetDevice_Type) override
   {
      abort();
   }
   /**
    * Append a technology_node to a liberty file.
    */
   void lib_write(std::ofstream&, const simple_indentRef) override
   {
      abort();
   }
   /**
    * Append a technology_node to a LEF file.
    */
   void lef_write(std::ofstream&, const simple_indentRef) override
   {
      abort();
   }
   /**
    * function that prints the class storage_unit.
    * @param os is the output stream.
    */
   void print(std::ostream&) const override
   {
      abort();
   }

   /**
    * Redefinition of get_kind_text()
    */
   GET_TEC_KIND_TEXT(storage_unit)
   /**
    * Redefinition of get_kind()
    */
   GET_TEC_KIND(storage_unit)

 private:
   friend class technology_manager;

   /**
    * @name default values associated with the members of the functional_unit class.
    */
   //@{
   static const double area_DEFAULT;
   //@}
};

#endif
