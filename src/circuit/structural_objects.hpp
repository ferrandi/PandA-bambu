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
 * @file structural_objects.hpp
 * @brief This class describes all classes used to represent a structural object.
 * Objects can represent items at different level of abstraction: logic, RTL and TLM.
 *
 * @author Matteo Barbati <mbarbati@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef STRUCTURAL_OBJECTS_HPP
#define STRUCTURAL_OBJECTS_HPP

/// Autoheader include
#include "config_HAVE_BAMBU_BUILT.hpp"
#include "config_HAVE_KOALA_BUILT.hpp"
#include "config_HAVE_TECHNOLOGY_BUILT.hpp"
#include "config_HAVE_TUCANO_BUILT.hpp"

/// STL include
#include <algorithm>
#include <utility>
#include <vector>

#include "custom_map.hpp"
#include "custom_set.hpp"

#include <ostream>
#include <string>

#include "simple_indent.hpp"
#include "utility.hpp"

#include "NP_functionality.hpp"
#include "exceptions.hpp"
#include "refcount.hpp"

/**
 * @name Forward declarations.
 */
//@{
#if HAVE_TUCANO_BUILT
REF_FORWARD_DECL(tree_manager);
#endif
#if HAVE_BAMBU_BUILT
CONSTREF_FORWARD_DECL(BehavioralHelper);
#endif
REF_FORWARD_DECL(structural_manager);

CONSTREF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(structural_object);
REF_FORWARD_DECL(structural_type_descriptor);
CONSTREF_FORWARD_DECL(technology_manager);
REF_FORWARD_DECL(technology_node);
REF_FORWARD_DECL(attribute);
/// forward decl of xml Element
class xml_element;
class simple_indent;

//@}

#define HIERARCHY_SEPARATOR "/"

#define MEMORY_PARAMETER "MEMORY_PARAMETER"
#define PIPE_PARAMETER "PIPE_PARAMETER"
#define VALUE_PARAMETER "VALUE_PARAMETER"

/// standard name for ports
#define CLOCK_PORT_NAME "clock"
#define WENABLE_PORT_NAME "wenable"
#define START_PORT_NAME "start_port"
#define RESET_PORT_NAME "reset"
#define DONE_PORT_NAME "done_port"
#define RETURN_PORT_NAME "return_port"
#define START_PORT_NAME_CFC "start_port_CFC"
#define DONE_PORT_NAME_CFC "done_port_CFC"
#define PRESENT_STATE_PORT_NAME "present_state"
#define NEXT_STATE_PORT_NAME "next_state"
#define NOTIFIER_PORT_MISMATCH "out_mismatch"
#define NOTIFIER_PORT_MISMATCH_ID "out_mismatch_id"
#define NOTIFIER_PORT_MISMATCH_OFFSET "out_mismatch_trace_offset"
#define SELECTOR_REGISTER_FILE "selector_register_file"
#define SUSPENSION "suspension"
#define REQUEST_ACCEPTED "request_accepted"
#define TASKS_POOL_END "task_pool_end"
#define DONE_SCHEDULER "done_scheduler"
#define DONE_REQUEST "done_request"
#define PROXY_PREFIX "PROXY_PREF_"
#define WRAPPED_PROXY_PREFIX "WRAPPED_PROXY_PREF_"

/**
 * Macro which defines the get_kind_text function that returns the parameter as a string.
 */
#define GET_SO_KIND_TEXT(meth)                \
   std::string get_kind_text() const override \
   {                                          \
      return std::string(#meth);              \
   }

/**
 * Structure representing the most relevant information about the type of a structural object.
 * In case the structural object is a signal or a port all member has a real meaning,
 * while when the structural object is a channel, component, event, data or an action the only relevant information
 * stored into the descriptor is the id_type and treenode when available.
 * The size member has also a relevant meaning when a channel, data, and an action? is considered.
 */
struct structural_type_descriptor
{
   /// Define the possible type of a structural object
   enum s_type
   {
      OTHER = 0,
      BOOL,
      INT,
      UINT,
      REAL,
      USER,
      VECTOR_BOOL,
      VECTOR_INT,
      VECTOR_UINT,
      VECTOR_REAL,
      VECTOR_USER,
      UNKNOWN
   };

   /// The type of the port or the signal
   s_type type;

   /// The size of the object (in bit). The objects having a size are: ports, signals, channels, data, and actions.
   unsigned int size;

   /// The number of the elements of a vector.
   unsigned int vector_size;

   /// Original type id of the structural object.
   std::string id_type;

   /// Treenode id of the type
   unsigned int treenode;

   /**
    * Constructor
    */
   structural_type_descriptor() : type(UNKNOWN), size(size_DEFAULT), vector_size(vector_size_DEFAULT), treenode(treenode_DEFAULT)
   {
   }

   /**
    * Object factory.
    * @param type_name is the string starting from which the object is built.
    * @param vector_size is the size of the the vector.
    *        In case vector_size is zero the descriptor type represents a scalar object,
    *        otherwise an array.
    */
   structural_type_descriptor(const std::string& type_name, unsigned int vector_size);

   /**
    * Object factory for module objects.
    * @param treenode is the treenode descriptor of the type.
    */
   explicit structural_type_descriptor(std::string module_name) : type(OTHER), size(size_DEFAULT), vector_size(vector_size_DEFAULT), id_type(std::move(module_name)), treenode(treenode_DEFAULT)
   {
   }

#if HAVE_TUCANO_BUILT
   /**
    * Object factory for SystemC objects.
    * @param treenode is the treenode descriptor of the type.
    */
   structural_type_descriptor(unsigned int treenode, tree_managerRef tm);
#endif

#if HAVE_BAMBU_BUILT
   /**
    * Object factory used in HLS
    * @param index is the index descriptor of the type
    * @param helper is the BehavioralHelper
    */
   structural_type_descriptor(unsigned int index, const BehavioralHelperConstRef helper);
#endif

   /**
    * Destructor
    */
   ~structural_type_descriptor() = default;

   /**
    * Method that copies the contents of the current structural_type_descriptorRef into another structural_type_descriptor
    * @param dest is the reference to the structural_type_descriptor where the contents have to be written
    */
   void copy(structural_type_descriptorRef dest);

   /**
    * Returns the name of the type descriptor
    */
   const std::string get_name() const;

   /**
    * Check if two type descriptors are consistent.
    * @param src_type is the first type descriptor.
    * @param dest_type is the second type descriptor.
    * @return true if the two type descriptors are consistent.
    */
   static bool check_type(structural_type_descriptorRef src_type, structural_type_descriptorRef dest_type);

   /**
    * Load a structural_type_descriptor starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    */
   void xload(const xml_element* Enode, structural_type_descriptorRef owner);

   /**
    * Add a structural_type_descriptor to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural type descriptor is attached.
    */
   void xwrite(xml_element* rootnode);

   /**
    * function that prints the class.
    * @param os is the output stream
    */
   void print(std::ostream& os) const;

   /**
    * Definition of get_kind_text()
    */
   std::string get_kind_text() const
   {
      return std::string("structural_type_descriptor");
   }

   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const structural_type_descriptorRef o)
   {
      if(o)
         o->print(os);
      return os;
   }

 private:
   /**
    * @name default values associated with the members of the structural_type_descriptor class.
    */
   //@{
   static const s_type type_DEFAULT = OTHER;
   static const unsigned int treenode_DEFAULT = 0;
   static const unsigned int size_DEFAULT = 0;
   static const unsigned int vector_size_DEFAULT = 0;
   //@}
   /// store the names of the enumerative s_type.
   static const char* s_typeNames[];
};

/**
 * Macro returning the string name of a type.
 */
#define GET_TYPE_NAME(structural_obj) ((structural_obj)->get_typeRef()->id_type)

/**
 * Macro returning the size of the type of a structural object.
 */
#define GET_TYPE_SIZE(structural_obj) ((structural_obj)->get_typeRef()->vector_size ? ((structural_obj)->get_typeRef()->vector_size * (structural_obj)->get_typeRef()->size) : (structural_obj)->get_typeRef()->size)

/**
 * Macro returning the size of a type
 */
#define STD_GET_SIZE(structural_obj) ((structural_obj)->vector_size ? ((structural_obj)->vector_size * (structural_obj)->size) : (structural_obj)->size)

/**
 * RefCount type definition of the structural_type_descriptor class structure
 */
typedef refcount<structural_type_descriptor> structural_type_descriptorRef;

/**
 * Enumerative type for structural object classes, it is used with get_kind() function
 * to know the actual type of a structural_object.
 */
enum so_kind
{
   component_o_K,
   channel_o_K,
   bus_connection_o_K,
   constant_o_K,
   signal_o_K,
   signal_vector_o_K,
   port_o_K,
   port_vector_o_K,
   event_o_K,
   data_o_K,
   action_o_K
};

/**
 * Macro used to implement get_kind() function in structural_object hyerarchy classes
 */
#define GET_SO_KIND(meth)                 \
   enum so_kind get_kind() const override \
   {                                      \
      return (meth##_K);                  \
   }

/**
 * Base object for all the structural objects.
 * It provides a common interface for each structural object present in a design.
 */
class structural_object
{
   /// The owner  of the object
   Wrefcount<structural_object> owner;

   /// Identifier for this component
   std::string id;

   /// The description of the type.
   structural_type_descriptorRef type;

   /// index of the treenode in the tree_manager associated with the structural object.
   unsigned int treenode;

   /// True if the structural object is a black box (e.g., a library component).
   bool black_box;

   /// Map between parameter string and related values of an instance
   CustomMap<std::string, std::string> parameters;

   /// Map between parameter string and its default value
   CustomMap<std::string, std::string> default_parameters;

 protected:
   /// debug level for the object
   int debug_level;

   /**
    * Convert a so_kind in a short string. Used in debugging.
    */
   std::string convert_so_short(so_kind in) const
   {
      switch(in)
      {
         case component_o_K:
            return "M";
         case channel_o_K:
            return "C";
         case constant_o_K:
            return "c";
         case bus_connection_o_K:
            return "B";
         case signal_o_K:
            return "S";
         case signal_vector_o_K:
            return "S";
         case port_o_K:
            return "P";
         case port_vector_o_K:
            return "P";
         case event_o_K:
            return "E";
         case data_o_K:
            return "D";
         case action_o_K:
            return "A";
         default:
            THROW_UNREACHABLE("");
      }
      return "";
   }
   /// pretty print functor object used by all print members to indent the output of the print function.
   static simple_indent PP;

#if HAVE_TECHNOLOGY_BUILT
   std::vector<std::string> attribute_list;

   std::map<std::string, attributeRef> attributes;
#endif

 public:
   /**
    * Constructor for the structural_object.
    * @param o is the owner (null object for the top object).
    */
   structural_object(int debug_level, const structural_objectRef o);

   /// virtual destructor
   virtual ~structural_object()
   {
   }

   /**
    * Return the owner.
    */
   const structural_objectRef get_owner() const;

   /**
    * set the owner of the structural object
    * @param new_owner is the owner (null object for the top object).
    */
   void set_owner(const structural_objectRef new_owner);

   /**
    * Set the treenode id associated with the structural_object.
    * @param n is the treenode id.
    */
   void set_treenode(unsigned int n);

   /**
    * Return the treenode id associated with the structural_object.
    */
   unsigned int get_treenode() const;

   /**
    * Set the identifier associated with the structural_object.
    * @param s is a string identifying the structural_object.
    */
   void set_id(const std::string& s);

#if HAVE_KOALA_BUILT
   /**
    * Return the equation associated with the output port of the component
    */
   std::string get_equation(const structural_objectRef out_obj, const technology_managerConstRef TM, CustomOrderedSet<structural_objectRef>& analyzed, const CustomOrderedSet<structural_objectRef>& input_ports,
                            const CustomOrderedSet<structural_objectRef>& output_ports) const;
#endif

   /**
    * Return the identifier associated with the structural_object.
    */
   const std::string get_id() const;

   /**
    * Set the type of the structural_object.
    * @param s is a type descriptor of a structural_object.
    */
   void set_type(const structural_type_descriptorRef& s);

   /**
    * Return the type descriptor of the structural_object
    */
   const structural_type_descriptorRef& get_typeRef() const;

   /**
    * Just resize the size of the bits of the object
    */
   void type_resize(unsigned int new_bit_size);

   /**
    * resizing of vector objects
    */
   void type_resize(unsigned int new_bit_size, unsigned int new_vec_size);
   /**
    * Set the black box property associated with the structural_object.
    * @param bb is true when the object is a black box, false otherwise.
    * The black box property has mean only in case of components and channels.
    */
   void set_black_box(bool bb);

   /**
    * Return the black box property.
    */
   bool get_black_box() const;

   /**
    * Set a parameter value
    * @param name is parameter name
    * @param value is parameter value
    */
   void SetParameter(const std::string& name, const std::string& value);

   /**
    * Check if a parameter has been specified
    * @param name is parameter name
    */
   bool ExistsParameter(std::string name) const;

   /**
    * Get the value associated to parameter if it has been associated; if it has not specified returns the default
    * @param name is parameter name
    * @return parameter value
    */
   std::string GetParameter(std::string name) const;

   /**
    * Get the value associated to parameter if it has been associate; It throws an exception if it has not
    * been associated
    * @param name is parameter name
    * @return parameter value
    */
   std::string GetDefaultParameter(std::string name) const;

   /**
    * return the whole set of parameters
    * @return the whole set of parameters
    */
   CustomMap<std::string, std::string> GetParameters();

   /**
    * Add a parameter
    * @param name is the name of the parameter
    * @param default_value is the default of the value
    */
   virtual void AddParameter(const std::string& name, const std::string& default_value);

   /**
    * Return a unique identifier of the structural object.
    * It is composed by the identifier of the current structural object
    * and by its owners separated by the HIERARCHY_SEPARATOR. Structural objects are viewed as elements of a standard filesystem.
    */
   const std::string get_path() const;

   /**
    * Perform a copy of the structural object.
    * @param dest destination object.
    */
   virtual void copy(structural_objectRef dest) const;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   virtual structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const = 0;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   virtual structural_objectRef find_isomorphic(const structural_objectRef key) const = 0;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   virtual void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM);

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   virtual void xwrite(xml_element* rootnode);

#if HAVE_TECHNOLOGY_BUILT
   /**
    * Add the list of attributes for the object
    * @param rootnode is the root node at which the xml representation of the attributes is attached
    */
   virtual void xwrite_attributes(xml_element* rootnode, const technology_nodeRef& tn = technology_nodeRef());
#endif

   /**
    * Print the structural_object (for debug purpose)
    * @param os is an output stream
    */
   virtual void print(std::ostream& os) const;

   /**
    * Friend definition of the << operator.
    */
   friend std::ostream& operator<<(std::ostream& os, const structural_objectRef o)
   {
      if(o)
         o->print(os);
      return os;
   }

   /**
    * Virtual function used to get the string name
    * of a structural_object instance.
    * @return a string identifying the object type.
    */
   virtual std::string get_kind_text() const = 0;
   /**
    * Virtual function used to find the real type
    * of a structural_object instance.
    * @return a so_kind enum identifying the object type.
    */
   virtual enum so_kind get_kind() const = 0;

#if HAVE_TECHNOLOGY_BUILT
   void add_attribute(const std::string& name, const attributeRef& attribute);

   attributeRef get_attribute(const std::string& name) const;

   const std::vector<std::string>& get_attribute_list() const;
#endif

 private:
   /**
    * @name default values associated with the members of the structural_type_descriptor class.
    */
   //@{
   static const unsigned int treenode_DEFAULT = 0;
   static const bool black_box_DEFAULT = true;
   //@}
};

/**
 * RefCount type definition of the structural_object class structure
 */
typedef refcount<structural_object> structural_objectRef;

/**
 * This class describes a port associated with a component or a channel.
 * A port can be in relation with:
 * - another standard port (e.g., primary input/output connections);
 * - a signal (e.g., standard internal connections):
 * - a port of a channel (e.g, when a channel is used as a connection).
 */
struct port_o : public structural_object
{
 public:
   /// Enumerative type describing the direction of a port.
   enum port_direction
   {
      IN = 0,
      OUT,
      IO,
      GEN,
      UNKNOWN,
      TLM_IN,
      TLM_OUT,
      TLM_INOUT
   };

   /// Enumerative type describing the endianess of a port; NONE means that it has not been specified yet.
   enum port_endianess
   {
      LITTLE = 0,
      BIG,
      NONE
   };

   /// Enumerative type describing if the port is associated with a specific interface type.
   enum port_interface
   {
      PI_DEFAULT = 0,
      PI_RNONE,
      PI_WNONE,
      PI_RACK,
      PI_WACK,
      PI_RVALID,
      PI_WVALID,
      PI_EMPTY_N,
      PI_READ,
      PI_FULL_N,
      PI_WRITE,
      PI_ADDRESS,
      PI_CHIPENABLE,
      PI_WRITEENABLE,
      PI_DIN,
      PI_DOUT
   };

   static const unsigned int PARAMETRIC_PORT = static_cast<unsigned int>(-1);

   /**
    * Convert a string into the corresponding port_direction enumerative type
    * @param val is the string version of the enum.
    */
   static port_direction to_port_direction(const std::string& val);

   /**
    * Convert a string into the corresponding port_interface enumerative type
    * @param val is the string version of the enum.
    */
   static port_interface to_port_interface(const std::string& val);

   /**
    * Constructor.
    * @param o is the owner of the port
    * @param dir defines the direction of the port.
    */
   port_o(int debug_level, const structural_objectRef o, port_direction dir, so_kind _port_type);

   /// Destructor.
   ~port_o() override = default;

   /// custom size parameter
   std::string size_parameter;

   /**
    * Bind this port with a signal/port
    * @param s is the connection
    */
   void add_connection(structural_objectRef s);

   /**
    * remove a connection (signal/port) from this
    * @param s is the connection to be removed
    */
   void remove_connection(structural_objectRef s);

   bool is_connected(structural_objectRef s) const;

   void substitute_connection(structural_objectRef old_conn, structural_objectRef new_conn);

   /**
    * Return connection bounded to this port
    * @param n is the index of the connection
    */
   const structural_objectRef get_connection(unsigned int n) const;

   /**
    * Return the number of connections bounded to this port
    */
   unsigned int get_connections_size() const;

   /**
    * Return the direction of the port.
    */
   port_direction get_port_direction() const;

   /**
    * Set the direction of the port.
    */
   void set_port_direction(port_direction dir);

   /**
    * Return the endianess of the port.
    */
   port_endianess get_port_endianess() const;

   /**
    * Set the endianess of the port.
    */
   void set_port_endianess(port_endianess end);

   /**
    * Return the port interface type of the port.
    */
   port_interface get_port_interface() const;

   /**
    * Set the port interface type of the port.
    */
   void set_port_interface(port_interface pi);

   /**
    * Return the port interface alignment.
    */
   unsigned get_port_alignment() const;

   /**
    * Set the port interface alignment.
    */
   void set_port_alignment(unsigned algn);

   /**
    * Return the connected signal, if any. Null pointer otherwise.
    */
   structural_objectRef get_connected_signal() const;

   /**
    * Set port size
    * @param dim is the new dimension of the port
    */
   void set_port_size(unsigned int dim);

   /**
    * Get port size
    * @return the dimension of the port
    */
   unsigned int get_port_size() const;

   /**
    * set the is var_args attribute.
    * @param c when true the port has a variable numbers of inputs
    */
   void set_is_var_args(bool c);
   /**
    * return true if the port is a var_args
    */
   bool get_is_var_args() const;

   /**
    * set the is clock attribute.
    * @param c when true the port is a clock port and has to be attached to a clock object
    */
   void set_is_clock(bool c);
   /**
    * return true if the port is a clock
    */
   bool get_is_clock() const;

   /**
    * set the is_extern attribute.
    * @param c when true the port is a extern port and has to be attached to an extern object
    */
   void set_is_extern(bool c);
   /**
    * return true if the port is extern
    */
   bool get_is_extern() const;

   /**
    * Sets the bus bundle identifier.
    */
   void set_bus_bundle(const std::string& name);

   /**
    * Returns the bus bundle identifier, if any
    */
   std::string get_bus_bundle() const;

   /**
    * set the is_global attribute.
    * @param c when true the port is a global port and has to be attached to a global object
    */
   void set_is_global(bool c);
   /**
    * return true if the port is global
    */
   bool get_is_global() const;

   /**
    * set the is_memory attribute.
    * @param c when true the port is a port used to interact with the onchip or offchip memory
    */
   void set_is_memory(bool c);
   /**
    * return true if the port is a memory port
    */
   bool get_is_memory() const;

   /**
    * set the is_slave attribute.
    * @param c when true the port is a port used to interact with the onchip or offchip memory as slave
    */
   void set_is_slave(bool c);
   /**
    * return true if the port works as slave memory port
    */
   bool get_is_slave() const;

   /**
    * set the is_master attribute.
    * @param c when true the port is a port used to interact with the onchip or offchip memory as master
    */
   void set_is_master(bool c);
   /**
    * return true if the port works as master memory port
    */
   bool get_is_master() const;

   /**
    * set the is_data attribute.
    * @param c when true the port is a port used as a data bus
    */
   void set_is_data_bus(bool c);

   /**
    * return true if the port works as a data bus
    */
   bool get_is_data_bus() const;

   /**
    * set the is_addr_bus attribute.
    * @param c when true the port is a port used as an address bus
    */
   void set_is_addr_bus(bool c);

   /**
    * return true if the port works as an address bus
    */
   bool get_is_addr_bus() const;

   /**
    * set the is_size_bus attribute.
    * @param c when true the port is a port used as a size bus
    */
   void set_is_size_bus(bool c);

   /**
    * return true if the port works as a size bus
    */
   bool get_is_size_bus() const;

   /**
    * set the is_tag_bus attribute.
    * @param c when true the port is a port used as an tag bus
    */
   void set_is_tag_bus(bool c);

   /**
    * return true if the port works as an tag bus
    */
   bool get_is_tag_bus() const;

   /**
    * set the is_doubled attribute.
    * @param c when true the port has a doubled size w.r.t the precision
    */
   void set_is_doubled(bool c);

   /**
    * return true if the port has a doubled size w.r.t the precision
    */
   bool get_is_doubled() const;

   /**
    * set the is_halved attribute.
    * @param c when true the port has a halved size w.r.t the precision
    */
   void set_is_halved(bool c);

   /**
    * return true if the port has a halved size w.r.t the precision
    */
   bool get_is_halved() const;

   /**
    * Find the object bounded to the port. The object searched has the same owner of the port.
    * @return the object bounded to the port.
    */
   structural_objectRef find_bounded_object(const structural_objectConstRef f_owner = structural_objectConstRef()) const;

   /**
    * set the port as critical with respect to the timing path
    */
   void set_critical();

   /**
    * return if the component is critical or not
    */
   bool get_critical() const;

   /**
    * Sets the port as reverse
    */
   void set_reverse();

   /**
    * Returns if the port as to be printed in a reverse mode
    */
   bool get_reverse() const;

   /**
    * Perform a copy of the port.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

#if HAVE_TECHNOLOGY_BUILT
   /**
    * Add all the port attributes to an xml tree.
    * @param rootnode is the root node at which the xml representation of attributes is attached.
    */
   void xwrite_attributes(xml_element* rootnode, const technology_nodeRef& tn = technology_nodeRef()) override;
#endif

   /**
    * Specify the number of ports of a generic port_vector object and add its corresponding ports.
    * It can be done only on port_vectors not yet specialized.
    * The type of the port is equal to the type descriptor of the port_vector.
    * @param n_ports is the number of the ports.
    * @param owner is the reference version of "this".
    */
   void add_n_ports(unsigned int n_ports, structural_objectRef owner);

   /**
    * Return the ith port of the vector. It checks that a port exists at position n.
    * @param n is the index of the port
    */
   const structural_objectRef get_port(unsigned int n) const;

   /**
    * Return the number of ports. When the port_vector has not yet specialized 0 is returned.
    */
   unsigned int get_ports_size() const;

   /**
    * return the index of the least significant port
    */
   unsigned int get_lsb() const
   {
      return lsb;
   }

   /**
    * auxiliary function used to resize the bus ports with respect to their associated bus size
    * @param bus_size_bitsize bitsize of sizes
    * @param bus_addr_bitsize bitsize of addresses
    * @param bus_data_bitsize bitsize of data
    * @param bus_tag_bitsize bitsize of tag
    * @param port is the port to be resized
    */
   static void resize_busport(unsigned int bus_size_bitsize, unsigned int bus_addr_bitsize, unsigned int bus_data_bitsize, unsigned int bus_tag_bitsize, structural_objectRef port);

   /**
    * auxiliary function used to resize the standard ports
    * @param bitsize_variable is the bitsize
    * @param n_elements is the number of elements
    * @param debug_level is the debug level
    * @param port is the port to be resized
    */
   static void resize_std_port(unsigned int bitsize_variable, unsigned int n_elements, int debug_level, structural_objectRef port);

   /**
    * copy the port properties from port_i to cir_port
    * @param port_i is the source port
    * @param cir_port is the created port
    */
   static void fix_port_properties(structural_objectRef port_i, structural_objectRef cir_port);

   /**
    * Print the port (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * return the name of the class as a string.
    */
   std::string get_kind_text() const override
   {
      if(port_type == port_vector_o_K)
         return "port_vector_o";
      else
         return "port_o";
   }
   /**
    * return the type of the class
    */
   enum so_kind get_kind() const override
   {
      return port_type;
   }

 private:
   /// The list of connections associated with the port.
   std::vector<Wrefcount<structural_object>> connected_objects;

   /// direction of a port
   port_direction dir;

   /// endianess of a port
   port_endianess end;

   /// port interface type of a port
   port_interface pi;

   unsigned aligment;

   /// when true the port must be specialized at runtime depending on the number of input
   bool is_var_args;

   /// when true the port is a clock port and has to be attached to a clock object
   bool is_clock;

   /// when true the port is an extern port
   bool is_extern;

   /// when true the port is a global port
   bool is_global;

   /// when true the port is dumped as [0:msb-1] instead of [msb-1:0]
   bool is_reverse;

   /// when true the port is a memory port
   bool is_memory;

   /// when true the port is a slave port
   bool is_slave;

   /// when true the port is a master port
   bool is_master;

   /// when true the port is a data bus
   bool is_data_bus;

   /// when true the port is an address bus
   bool is_addr_bus;

   /// when true the port is a size bus
   bool is_size_bus;

   /// when true the port is a tag bus
   bool is_tag_bus;

   /// when true the port has a doubled size
   bool is_doubled;

   /// when true the port has a halfed size
   bool is_halved;

   /// when true the port is involved into the critical path of the netlist
   bool is_critical;

   /// bus bundle
   std::string bus_bundle;

   /// least significant bit
   unsigned int lsb;

   /// The list of ports associated with the port.
   std::vector<structural_objectRef> ports;

   /// store the names of the enumerative port_direction.
   static const char* port_directionNames[];

   /// store the names of the enumerative port_interface.
   static const char* port_interfaceNames[];

   /// port type
   so_kind port_type;

   /**
    * @name default values associated with the members of the port_o class.
    */
   //@{
   static const bool is_clock_DEFAULT = false;
   static const bool is_extern_DEFAULT = false;
   static const bool is_global_DEFAULT = false;
   static const bool is_memory_DEFAULT = false;
   static const bool is_slave_DEFAULT = false;
   static const bool is_master_DEFAULT = false;
   static const bool is_data_bus_DEFAULT = false;
   static const bool is_addr_bus_DEFAULT = false;
   static const bool is_tag_bus_DEFAULT = false;
   static const bool is_size_bus_DEFAULT = false;
   static const bool is_doubled_DEFAULT = false;
   static const bool is_halved_DEFAULT = false;
   static const bool is_critical_DEFAULT = false;
   static const bool is_reverse_DEFAULT = false;
   static const bool is_var_args_DEFAULT = false;
   static const unsigned port_interface_alignment_DEFAULT = 0;
   //@}
};

/**
 * This class describes a generic event.
 */
class event_o : public structural_object
{
 public:
   /**
    * Constructor.
    * @param o is the owner of the event
    */
   event_o(int debug_level, const structural_objectRef o);

   /// Destructor
   ~event_o() override = default;

   /**
    * Perform a copy of the event.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * This method throw and error since does not have associated any structural object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

   /**
    * Print the event (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_SO_KIND_TEXT(event_o)
   /**
    * Redefinition of get_kind()
    */
   GET_SO_KIND(event_o)
};

/**
 * This class describes a generic data declaration object.
 */
class data_o : public structural_object
{
 public:
   /**
    * Constructor.
    * @param o is the owner of the data declaration object.
    */
   data_o(int debug_level, const structural_objectRef o);

   /// destructor
   ~data_o() override = default;

   /**
    * Perform a copy of the data.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * This method throw and error since does not have associated any structural object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

   /**
    * Print the data declaration object (for debug purpose).
    * @param os is an output stream.
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_SO_KIND_TEXT(data_o)
   /**
    * Redefinition of get_kind()
    */
   GET_SO_KIND(data_o)
};

/**
 * This class describes a generic systemC action.
 * An action can be a SystemC process or a standard service (aka a member function of a class).
 */
class action_o : public structural_object
{
 public:
   /**
    * Define the possible types of a process.
    */
   enum process_type
   {
      THREAD = 0,
      CTHREAD,
      METHOD,
      SERVICE,
      UNKNOWN
   };

 private:
   /// The method procedure parameter.
   std::vector<structural_objectRef> parameters;

   /// The index of the function which represents the behavior.
   unsigned int function_id;

   /// The type of the action.
   process_type action_type;

   /// Sensitivity list.
   std::vector<structural_objectRef> action_sensitivity;

   /// Used to identify the scope of the action (public, private or protected)
   std::string scope;

 public:
   /**
    * Constructor.
    * @param o is the owner of the action.
    */
   action_o(int debug_level, const structural_objectRef o);

   /// destructor
   ~action_o() override = default;

   /**
    * Add a procedure paramenter.
    * @param d is the paramenter.
    */
   void add_parameter(structural_objectRef d);

   /**
    * Return the ith input parameter.
    * @param n is the index of the input parameter.
    */
   const structural_objectRef get_parameter(unsigned int n) const;

   /**
    * Return the number of the parameters.
    */
   unsigned int get_parameters_size() const;

   /**
    * Function that set the function with the action.
    * @param id is the index of the function
    */
   void set_fun_id(unsigned int id);

   /**
    * Return the function id
    * @return the index of the function associated with the action
    */
   unsigned int get_fun_id() const;

   /**
    * Set the type of the action.
    * @param at is the type of the action.
    */
   void set_action_type(process_type at);

   /**
    * Return the type of the action.
    */
   process_type get_action_type() const;

   /**
    * Add an event to the sensitivity list of the action.
    * @param e is an event.
    */
   void add_event_to_sensitivity(structural_objectRef e);

   /**
    * Return tha size of the sensitivity list.
    */
   unsigned int get_sensitivity_size() const;

   /**
    * Return a specific event of the sensitivity list.
    * @param n is the index of the event.
    */
   const structural_objectRef get_sensitivity(unsigned int n) const;

   /**
    * Set the scope of the action.
    * @param sc is the actual scope.
    */
   void set_scope(const std::string& sc);

   /**
    * Return the scope of the action.
    */
   const std::string& get_scope() const;

   /**
    * Return the type of the action: process (true) or services (false).
    */
   bool get_process_nservice() const;

   /**
    * Perform a copy of the action.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

   /**
    * Print the action (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_SO_KIND_TEXT(action_o)
   /**
    * Redefinition of get_kind()
    */
   GET_SO_KIND(action_o)

 private:
   /// store the names of the enumerative process_type.
   static const char* process_typeNames[];
};

/**
 * This class describes a constant value.
 * The value is defined by a size and a value
 */
class constant_o : public structural_object
{
   /// Value of this element
   std::string value;

   /// List of ports bounded by the constant object.
   std::vector<Wrefcount<structural_object>> connected_objects;

 public:
   /**
    * Constructor.
    * @param o is the owner of the value
    */
   constant_o(int debug_level, const structural_objectRef o);

   /**
    * Constructor.
    * @param o is the owner of the value
    * @param value is the constant value
    */
   constant_o(int debug_level, const structural_objectRef o, std::string value);

   /// Destructor
   ~constant_o() override = default;

   /**
    * Bind the element object with a port/signal.
    * @param p is the port
    */
   void add_connection(structural_objectRef p);

   /**
    * Return the ith element bounded to the connection.
    * @param n is the index of the port.
    */
   structural_objectRef get_connection(unsigned int n) const;

   /**
    * Return the number of ports associated with the connection
    */
   unsigned int get_connected_objects_size() const;

   /**
    * Return the size associated with this element (in bits)
    */
   unsigned int get_size() const;

   /**
    * Return the (integer) value associated with this element
    */
   std::string get_value() const;

   /**
    * Perform a copy of the value.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

   /**
    * Print the constant value (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_SO_KIND_TEXT(constant_o)
   /**
    * Redefinition of get_kind()
    */
   GET_SO_KIND(constant_o)
};

/**
 * This class describes a simple logic/RTL signal.
 * A signal can be an array of bit but it cannot be sliced or partially accessed.
 * In case of partial access or slicing the bus_connection_o object should be used.
 */
class signal_o : public structural_object
{
 public:
   static const unsigned int PARAMETRIC_SIGNAL = static_cast<unsigned int>(-1);

   /**
    * Constructor.
    * @param o is the owner of the port
    */
   signal_o(int debug_level, const structural_objectRef o, so_kind _signal_type);

   /// Destructor
   ~signal_o() override = default;

   /**
    * Bind the connection object with a port.
    * @param p is the port
    */
   void add_port(structural_objectRef p);

   /**
    * remove the connection of this signal with a port
    * @param s is the connection
    */
   void remove_port(structural_objectRef s);

   bool is_connected(structural_objectRef s) const;

   void substitute_port(structural_objectRef old_port, structural_objectRef new_port);

   /**
    * set the signal as critical with respect to the timing path
    */
   void set_critical();

   /**
    * return if the component is critical or not
    */
   bool get_critical() const;

   /**
    * Return the ith port bounded to the connection.
    * @param n is the index of the port.
    */
   const structural_objectRef get_port(unsigned int n) const;

   /**
    * Return the ith port bounded to the connection.
    * @param n is the index of the port.
    */
   structural_objectRef get_port(unsigned int n);

   /**
    * Return the number of ports associated with the connection
    */
   unsigned int get_connected_objects_size() const;

   /**
    * Return if signal has both input and output
    * @return true if signal has both input and output
    */
   bool is_full_connected() const;

   /**
    * Specify the number of ports of a generic port_vector object and add its corresponding ports.
    * It can be done only on port_vectors not yet specialized.
    * The type of the port is equal to the type descriptor of the port_vector.
    * @param n_signals is the number of signals.
    * @param owner is the reference version of "this".
    */
   void add_n_signals(unsigned int n_signals, structural_objectRef owner);

   /**
    * Return the ith port of the vector. It checks that a port exists at position n.
    * @param n is the index of the port
    */
   const structural_objectRef get_signal(unsigned int n) const;

   /**
    * Return the ith port of the vector with respect to lsb. It also checks if the port exists
    * @param n is the index of the port
    */
   const structural_objectRef get_positional_signal(unsigned int n) const;

   /**
    * Return the number of ports. When the port_vector has not yet specialized 0 is returned.
    */
   unsigned int get_signals_size() const;

   /**
    * return the index of the least significant port
    */
   unsigned int get_lsb() const
   {
      return lsb;
   }

   /**
    * Perform a copy of the signal.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

   /**
    * Print the signal (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * return the name of the class as a string.
    */
   std::string get_kind_text() const override
   {
      if(signal_type == signal_vector_o_K)
         return "signal_vector_o";
      else
         return "signal_o";
   }
   /**
    * return the type of the class
    */
   enum so_kind get_kind() const override
   {
      return signal_type;
   }

 private:
   /// List of ports bound to the signal object.
   std::vector<structural_objectRef> connected_objects;

   /// when true the signal is involved into the critical path of the netlist
   bool is_critical;

   /// The list of signals associated with the vector of signals.
   std::vector<structural_objectRef> signals_;

   unsigned int lsb;

   /// port type
   so_kind signal_type;
};

/**
 * This class describes a generic module.
 * A module can be further specialized in a channel or in a component.
 * This class should be not instantiated. Only channels and components can be instantiated.
 */
class module : public structural_object
{
   /// input port of this module
   std::vector<structural_objectRef> in_ports;

   /// output ports of this module
   std::vector<structural_objectRef> out_ports;

   /// Input-output ports of this module
   std::vector<structural_objectRef> in_out_ports;

   /// generic ports of this module
   std::vector<structural_objectRef> gen_ports;

   /// internal components/channels/signals/bus_connection_os (for structural modules)
   std::vector<structural_objectRef> internal_objects;

   /// List of local data associated with the module.
   std::vector<structural_objectRef> local_data;

   /// List of event associated with the module.
   std::vector<structural_objectRef> list_of_event;

   /// List of processes associated with the module.
   std::vector<structural_objectRef> list_of_process;

   /// List of services associated with the module.
   std::vector<structural_objectRef> list_of_service;

   /// Alternative descriptions of the behavior of the module.
   NP_functionalityRef NP_descriptions;

   /// store the last index of the positional binding
   unsigned int last_position_port;

   /// when true the component is involved into the critical path of the netlist
   bool is_critical;

   /// when true the component has been internally generated
   bool is_generated;

   /// positional map, given the index return the port in that position
   std::map<unsigned int, structural_objectRef> positional_map;

   /// index for signals
   /// this table is used to quickly search internal signals used by find_member and find_isomorphic
   std::map<std::string, structural_objectRef> index_signals;

   /// index for constants
   /// this table is used to quickly search internal constants used by find_member and find_isomorphic
   std::map<std::string, structural_objectRef> index_constants;

   /// index for components
   /// this table is used to quickly search internal components used by find_member and find_isomorphic
   std::map<std::string, structural_objectRef> index_components;

   /// index for channels
   /// this table is used to quickly search internal channels used by find_member and find_isomorphic
   std::map<std::string, structural_objectRef> index_channels;

   /// index for bus_connections
   /// this table is used to quickly search internal bus_connections used by find_member and find_isomorphic
   std::map<std::string, structural_objectRef> index_bus_connections;

   /// Store the module description
   std::string description;

   /// Store the copyright description
   std::string copyright;

   /// Store the list of authors
   std::string authors;

   /// Store some tags concerning the license type associated with the functional unit
   std::string license;

   /// when non empty it defines with respect what module has been specialized
   std::string specialized;

   /// multi-unit multiplicity is the number of units implemented by this module all doing the same thing
   unsigned int multi_unit_multiplicity;

 public:
   /**
    * Constructor.
    * @param o is the owner of the module.
    */
   module(int debug_level, const structural_objectRef o);

   /// destructor
   ~module() override = default;

   /**
    * Return the total number of the ports
    */
   unsigned int get_num_ports() const;

   /**
    * return a port of the module given its position
    * @param index is the position of the port.
    */
   structural_objectRef get_positional_port(unsigned int index) const;

   /**
    * Add an input port.
    * @param p is the port.
    */
   void add_in_port(structural_objectRef p);

   /**
    * Return the ith input port.
    * @param n is the index of the port.
    */
   const structural_objectRef get_in_port(unsigned int n) const;

   /**
    * Return the number of input ports.
    */
   unsigned int get_in_port_size() const;

   /**
    * Add an output port.
    * @param p is the port .
    */
   void add_out_port(structural_objectRef p);

   /**
    * Return the ith output port.
    * @param n is the index of the port
    */
   const structural_objectRef get_out_port(unsigned int n) const;

   /**
    * Return the number of output ports.
    */
   unsigned int get_out_port_size() const;

   /**
    * Add an input-output port.
    * @param p is the port.
    */
   void add_in_out_port(structural_objectRef p);

   /**
    * Return the ith input-output port.
    * @param n is the index of the port.
    */
   const structural_objectRef get_in_out_port(unsigned int n) const;

   /**
    * Return the number of output ports.
    */
   unsigned int get_in_out_port_size() const;

   /**
    * Add a generic port.
    * @param p is the port.
    */
   void add_gen_port(structural_objectRef p);

   /**
    * Return the ith generic port.
    * @param n is the index of the port
    */
   const structural_objectRef get_gen_port(unsigned int n) const;

   /**
    * Return the number of generic ports.
    */
   unsigned int get_gen_port_size() const;

   /**
    * remove a port from the module
    * @param id is the name of the port
    */
   void remove_port(const std::string& id);

   /**
    * Add an internal component/channel/signal/bus_connection_o.
    * @param c can be a component, channel, signal or a bus_connection_o.
    */
   void add_internal_object(structural_objectRef c);

   void remove_internal_object(structural_objectRef c);

   /**
    * Return the ith internal objects.
    * @param n is the index of the objects.
    */
   const structural_objectRef get_internal_object(unsigned int n) const;

   /**
    * Return the number of internal objects
    */
   unsigned int get_internal_objects_size() const;

   /**
    * Add a process to the module.
    * @param p is the process.
    */
   void add_process(structural_objectRef p);

   /**
    * Return the ith process.
    * @param n is the index of the process.
    */
   const structural_objectRef get_process(unsigned int n) const;

   /**
    * Return the number of internal processes.
    */
   unsigned int get_process_size() const;

   /**
    * Add a service to the module.
    * @param p is the service.
    */
   void add_service(structural_objectRef p);

   /**
    * Return the ith service.
    * @param n is the index of the service.
    */
   const structural_objectRef get_service(unsigned int n) const;

   /**
    * Return the number of services.
    */
   unsigned int get_service_size() const;

   /**
    * Add an event to the module.
    * @param e is the event.
    */
   void add_event(structural_objectRef e);

   /**
    * Return the ith event.
    * @param n is the index of the event.
    */
   const structural_objectRef get_event(unsigned int n) const;

   /**
    * Return the number of events.
    */
   unsigned int get_event_size() const;

   /**
    * Add a local data.
    * @param d is the local data.
    */
   void add_local_data(structural_objectRef d);

   /**
    * Return the ith local data.
    * @param n represent the index of the local data.
    */
   const structural_objectRef get_local_data(unsigned int n) const;

   /**
    * Return the number of local data.
    */
   unsigned int get_local_data_size() const;

   /**
    * Set the alternative module behavior descriptions (Non SystemC based).
    * @param f is the alternative functionalities
    */
   void set_NP_functionality(NP_functionalityRef f);

   /**
    * Return the alternative functionalities.
    */
   const NP_functionalityRef& get_NP_functionality() const;

   /**
    * Return the list of object that can be parametrized.
    * This function is usually used by the backend.
    * @param owner is the refcount version of this.
    */
   void get_NP_library_parameters(structural_objectRef owner, std::vector<std::pair<std::string, structural_objectRef>>& parameters) const;

   /**
    * Perform a copy of the module.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override = 0;

#if HAVE_TECHNOLOGY_BUILT
   /**
    * Add the list of attributes for the component
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite_attributes(xml_element* rootnode, const technology_nodeRef& tn = technology_nodeRef()) override = 0;
#endif

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override = 0;

   /**
    * True if one of the ports of the module has the attribute is_var_args=true
    */
   bool is_var_args() const;

   /**
    * Print the module (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * set the component as critical with respect to the timing path
    */
   void set_critical();

   /**
    * return if the component is critical or not
    */
   bool get_critical() const;

   /**
    * set the component as generated
    */
   void set_generated();

   /**
    * return if the component has been generated or not
    */
   bool get_generated() const;

   /**
    * @brief set_multi_unit_multiplicity
    * @param value is the number of units implemented by this module all doing the same thing
    */
   void set_multi_unit_multiplicity(unsigned int value);

   /**
    * @brief get_multi_unit_multiplicity
    * @return the number of units implemented by this module
    */
   unsigned int get_multi_unit_multiplicity() const;

   /**
    * change the direction of a port
    * @param port is the port to be moved
    * @param pdir is the new direction
    */
   void change_port_direction(structural_objectRef port, port_o::port_direction pdir);

   /**
    * Return the description associated with the module
    */
   const std::string get_description() const
   {
      return description;
   }

   /**
    * Set the description associated with the module
    */
   void set_description(const std::string& d)
   {
      description = d;
   }

   /**
    * Return the copyright associated with the module
    */
   const std::string get_copyright() const
   {
      return copyright;
   }

   /**
    * Set the copyright associated with the module
    */
   void set_copyright(const std::string& c)
   {
      copyright = c;
   }

   /**
    * Return the authors of the functional description of the module
    */
   const std::string get_authors() const
   {
      return authors;
   }

   /**
    * Set the authors associated with the module
    */
   void set_authors(const std::string& a)
   {
      authors = a;
   }

   /**
    * Return the license of the functional description of the module
    */
   const std::string get_license() const
   {
      return license;
   }

   /**
    * Set the license associated with the module
    */
   void set_license(const std::string& l)
   {
      license = l;
   }

   /**
    * Return a non-empty string when the component has been specialized.
    * The string identify with respect what the component has been specialized (e.g., target_device, behavior...)
    */
   const std::string get_specialized() const
   {
      return specialized;
   }

   /**
    * Set the specialization string
    */
   void set_specialized(const std::string& s)
   {
      specialized = s;
   }

#if HAVE_TECHNOLOGY_BUILT
   /**
    * Return the type of a parameter
    * @param TM is the technology manager
    * @param name is the name of the parameter
    * @return the type of the parameter
    */
   structural_type_descriptor::s_type get_parameter_type(const technology_managerConstRef TM, const std::string& name) const;

   /**
    * Return the generic type of a module instance
    * @param TM is the technology manager
    * @return the generic structural object
    */
   structural_objectRef get_generic_object(const technology_managerConstRef TM) const;
#endif

   /**
    * Add a parameter
    * @param name is the name of the parameter
    * @param default_value is the default of the value
    */
   void AddParameter(const std::string& name, const std::string& default_value) override;
};

/**
 * This class describes a generic component.
 */
class component_o : public module
{
 public:
   /**
    * Constructor.
    * @param o is the owner of the component.
    */
   component_o(int debug_level, const structural_objectRef o);

   /// destructor
   ~component_o() override = default;

   /**
    * Perform a copy of the component.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

#if HAVE_TECHNOLOGY_BUILT
   /**
    * Add the list of attributes for the component
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite_attributes(xml_element* rootnode, const technology_nodeRef& tn = technology_nodeRef()) override;
#endif

   /**
    * Print the component (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_SO_KIND_TEXT(component_o)
   /**
    * Redefinition of get_kind()
    */
   GET_SO_KIND(component_o)
};

/**
 * This class describes a generic channel.
 */
class channel_o : public module
{
   /// List of the interfaces associated with the channel.
   std::map<unsigned int, std::string> impl_interfaces;

   /// List of ports bounded by the channel object.
   std::vector<structural_objectRef> connected_objects;

 public:
   /**
    * Constructor.
    * @param o is the owner of the channel.
    */
   channel_o(int debug_level, const structural_objectRef o);

   /// Destructor
   ~channel_o() override = default;

   /**
    * Add an interface to the object.
    */
   void add_interface(unsigned int t, const std::string& _interface);

   /**
    * Return the interface with a given treenode
    * @param n is the treenode of the interface
    */
   const std::string& get_interface(unsigned int t) const;

   /**
    * Bind the connection object with a port.
    * @param p is the port
    */
   void add_port(structural_objectRef p);

   /**
    * Return the ith port bounded to the connection.
    * @param n is the index of the port.
    */
   const structural_objectRef get_port(unsigned int n) const;

   /**
    * Return the number of ports associated with the connection
    */
   unsigned int get_connected_objects_size() const;

   /**
    * Perform a copy of the channel.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

#if HAVE_TECHNOLOGY_BUILT
   /**
    * Add the list of attributes for the component
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite_attributes(xml_element* rootnode, const technology_nodeRef& tn = technology_nodeRef()) override;
#endif

   /**
    * Print the channel (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_SO_KIND_TEXT(channel_o)
   /**
    * Redefinition of get_kind()
    */
   GET_SO_KIND(channel_o)
};

/**
 * This class describes a generic bus connection. A bus_connection_o is a a vector
 * of signals or channels [1..n].
 */
class bus_connection_o : public structural_object
{
   /// List of connections associated with the bus.
   std::vector<structural_objectRef> connections;

 public:
   /**
    * Constructor.
    * @param o is the owner of the bus_connection_o.
    */
   bus_connection_o(int debug_level, const structural_objectRef o);

   /// destructor
   ~bus_connection_o() override = default;

   /**
    * Add a connection (e.g. a signal or a channel) to the bus connection object.
    * @param c is a signal connection
    */
   void add_connection(structural_objectRef c);

   /**
    * Return a connection.
    * @param n is the index of the connection
    */
   const structural_objectRef get_connection(unsigned int n) const;

   /**
    * Return the number of connections associated with the bus connection object.
    */
   unsigned int get_connections_size() const;

   /**
    * Perform a copy of the bus_connection_o.
    * @param dest destination object.
    */
   void copy(structural_objectRef dest) const override;

   /**
    * Return the object named id of a given type which belongs to or it is associated with the object.
    * @param id is the identifier of the object we are looking for.
    * @param type is the type of the object we are looking for.
    * @param owner is the owner of the object named id.
    */
   structural_objectRef find_member(const std::string& id, so_kind type, const structural_objectRef owner) const override;

   /**
    * Find key in this object.
    * @param key is the object searched.
    */
   structural_objectRef find_isomorphic(const structural_objectRef key) const override;

   /**
    * Load a structural_object starting from an xml file.
    * @param node is a node of the xml tree.
    * @param owner is the refcount version of this.
    * @param CM is the circuit manager.
    */
   void xload(const xml_element* Enode, structural_objectRef owner, structural_managerRef const& CM) override;

   /**
    * Add a structural_object to an xml tree.
    * @param rootnode is the root node at which the xml representation of the structural object is attached.
    */
   void xwrite(xml_element* rootnode) override;

   /**
    * Print the bus_connection_o (for debug purpose)
    * @param os is an output stream
    */
   void print(std::ostream& os) const override;

   /**
    * Redefinition of get_kind_text()
    */
   GET_SO_KIND_TEXT(bus_connection_o)
   /**
    * Redefinition of get_kind()
    */
   GET_SO_KIND(bus_connection_o)
};

#endif
