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
 * @file structural_manager.cpp
 * @brief Implementation of the class that manages the creation of the graph associated with the circuit
 *
 * This class implements functions which build the circuit and the graph associated with the circuit
 *
 * @author Matteo Barbati <mbarbati@gmail.com>
 * @author Christian Pilato <pilato@elet.polimi.it>
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 * $Revision$
 * $Date$
 * Last modified by $Author$
 *
 */
#include "structural_manager.hpp"

#include "config_HAVE_BAMBU_BUILT.hpp"      // for HAVE_BAM...
#include "config_HAVE_KOALA_BUILT.hpp"      // for HAVE_KOA...
#include "config_HAVE_TECHNOLOGY_BUILT.hpp" // for HAVE_TEC...

#include "Parameter.hpp"                        // for Parameter
#include "cg_node.hpp"                          // for structur...
#include "custom_map.hpp"                       // for map, _Rb...
#include "custom_set.hpp"                       // for set, set...
#include "dbgPrintHelper.hpp"                   // for DEBUG_LE...
#include "exceptions.hpp"                       // for THROW_AS...
#include "graph.hpp"                            // for graphs_c...
#include "graph_info.hpp"                       // for GraphInf...
#include "library_manager.hpp"                  // for attribute
#include "refcount.hpp"                         // for GetPointer
#include <boost/algorithm/string/predicate.hpp> // for starts_with
#include <boost/graph/adjacency_list.hpp>       // for target
#include <boost/graph/filtered_graph.hpp>       // for edges
#include <boost/graph/graph_traits.hpp>         // for graph_tr...
#include <boost/graph/graphviz.hpp>             // for write_gr...
#include <boost/iterator/iterator_facade.hpp>   // for operator!=
#include <boost/smart_ptr/shared_ptr.hpp>       // for shared_ptr
#include <iosfwd>                               // for ofstream
#include <list>                                 // for _List_co...
#include <memory>                               // for allocato...
#include <ostream>                              // for operator<<
#include <utility>                              // for swap, pair
#include <vector>                               // for vector
#if HAVE_BAMBU_BUILT || HAVE_KOALA_BUILT || HAVE_EUCALYPTUS_BUILT
#include "technology_manager.hpp" // for technolo...
#include "technology_node.hpp"    // for function...
#endif
#include "string_manipulation.hpp" // for GET_CLASS
#include "typed_node_info.hpp"     // for ENTRY, EXIT
#include "xml_element.hpp"         // for xml_element
#include "xml_node.hpp"            // for xml_node...

structural_manager::structural_manager(const ParameterConstRef _Param) : Param(_Param), debug_level(_Param->get_class_debug_level(GET_CLASS(*this)))
{
   og = new graphs_collection(GraphInfoRef(new cg_graph_info), _Param);
   data_graph = new graph(og, PURE_DATA_SELECTOR);
   circuit_graph = new graph(og, ALL_LINES_SELECTOR);
}

structural_manager::~structural_manager()
{
   delete og;
   delete data_graph;
   delete circuit_graph;
}

bool structural_manager::check_type(structural_objectRef src_type, structural_objectRef dest_type)
{
   return structural_type_descriptor::check_type(src_type->get_typeRef(), dest_type->get_typeRef());
}

bool structural_manager::check_object(std::string id, structural_objectRef owner, so_kind type)
{
   THROW_ASSERT(id.find(HIERARCHY_SEPARATOR, 0) == std::string::npos, std::string("Object id cannot contain the \"/\" character"));
   return owner->find_member(id, type, owner) != nullptr;
}

bool structural_manager::check_bound(structural_objectRef src, structural_objectRef sign)
{
   auto* p = GetPointer<port_o>(src);
   for(unsigned int i = 0; i < p->get_connections_size(); i++)
      if(p->get_connection(i) == sign)
         return true;
   return false;
}

void structural_manager::set_top_info(std::string id, structural_type_descriptorRef module_type, unsigned int treenode)
{
   structural_objectRef nullobj;
   circuit = structural_objectRef(new component_o(debug_level, nullobj));
   auto* circ = GetPointer<component_o>(circuit);
   circ->set_id(id);
   circ->set_treenode(treenode);
   circ->set_type(module_type);
}

#if HAVE_BAMBU_BUILT || HAVE_KOALA_BUILT
void structural_manager::set_top_info(const std::string& id, const technology_managerRef& LM, const std::string& Library)
{
   structural_objectRef nullobj;
   circuit = this->add_module_from_technology_library(id, id, Library, nullobj, LM);
}
#endif

structural_objectRef structural_manager::create(std::string id, so_kind ctype, structural_objectRef owner, structural_type_descriptorRef obj_type, unsigned int treenode)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(!check_object(id, owner, ctype), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef cc;
   switch(ctype)
   {
      case component_o_K:
      {
         {
            cc = structural_objectRef(new component_o(debug_level, owner));
         }
         break;
      }
      case constant_o_K:
      {
         {
            cc = structural_objectRef(new constant_o(debug_level, owner));
         }
         break;
      }
      case channel_o_K:
      {
         {
            cc = structural_objectRef(new channel_o(debug_level, owner));
         }
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR(std::string("ctype not supported"));
   }
   cc->set_id(id);
   cc->set_treenode(treenode);
   cc->set_type(obj_type);
   auto* own = GetPointer<module>(owner);
   THROW_ASSERT(own, "Signal, port or interface couldn't own internal object");
   own->add_internal_object(cc);
   return cc;
}

structural_objectRef structural_manager::add_port(std::string id, port_o::port_direction pdir, structural_objectRef owner, structural_type_descriptorRef type_descr, unsigned int treenode)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(!check_object(id, owner, port_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef cp = structural_objectRef(new port_o(debug_level, owner, pdir, port_o_K));
   cp->set_id(id);
   cp->set_treenode(treenode);
   cp->set_type(type_descr);
   switch(owner->get_kind())
   {
      case channel_o_K:
      case component_o_K:
      {
         auto* own = GetPointer<module>(owner);
         switch(pdir)
         {
            case port_o::IN:
            {
               own->add_in_port(cp);
#if HAVE_TECHNOLOGY_BUILT
               attributeRef direction(new attribute(attribute::STRING, "input"));
               cp->add_attribute("direction", direction);
#endif
               break;
            }
            case port_o::OUT:
            {
               own->add_out_port(cp);
#if HAVE_TECHNOLOGY_BUILT
               attributeRef direction(new attribute(attribute::STRING, "output"));
               cp->add_attribute("direction", direction);
#endif
               if(GetPointer<module>(owner))
               {
                  NP_functionalityRef NPF = GetPointer<module>(owner)->get_NP_functionality();
                  if(NPF)
                  {
#if HAVE_TECHNOLOGY_BUILT
                     std::string equation = NPF->get_NP_functionality(NP_functionality::EQUATION);
                     std::vector<std::string> tokens = SplitString(equation, ";");
                     for(auto& token : tokens)
                     {
                        if(boost::algorithm::starts_with(token, id))
                           equation = token.substr(token.find("=") + 1, token.size());
                     }
                     attributeRef function(new attribute(attribute::STRING, equation));
                     cp->add_attribute("function", function);
#endif
                  }
               }
               break;
            }
            case port_o::IO:
            {
               own->add_in_out_port(cp);
               break;
            }
            case port_o::GEN:
            {
               own->add_gen_port(cp);
               break;
            }
            case port_o::TLM_IN:
            case port_o::TLM_INOUT:
            case port_o::TLM_OUT:
            case port_o::UNKNOWN:
            default:
               THROW_ERROR(std::string("port type not supported"));
         }
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR(std::string("Signal, port or interface couldn't own a port"));
   }

   return cp;
}

void structural_manager::change_port_direction(structural_objectRef port_object, port_o::port_direction pdir, structural_objectRef owner)
{
   switch(owner->get_kind())
   {
      case channel_o_K:
      case component_o_K:
      {
         auto* own = GetPointer<module>(owner);
         own->change_port_direction(port_object, pdir);
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR(std::string("object not yet managed"));
   }
}

structural_objectRef structural_manager::add_port_vector(std::string id, port_o::port_direction pdir, unsigned int n_ports, structural_objectRef owner, structural_type_descriptorRef type_descr, unsigned int treenode)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(!check_object(id, owner, port_vector_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef cp = structural_objectRef(new port_o(debug_level, owner, pdir, port_vector_o_K));
   cp->set_id(id);
   cp->set_treenode(treenode);
   cp->set_type(type_descr);
   if(n_ports != port_o::PARAMETRIC_PORT)
      GetPointer<port_o>(cp)->add_n_ports(n_ports, cp);
   switch(owner->get_kind())
   {
      case channel_o_K:
      case component_o_K:
      {
         auto* own = GetPointer<module>(owner);
         switch(pdir)
         {
            case port_o::IN:
            {
               own->add_in_port(cp);
               break;
            }
            case port_o::OUT:
            {
               own->add_out_port(cp);
               break;
            }
            case port_o::IO:
            {
               own->add_in_out_port(cp);
               break;
            }
            case port_o::GEN:
            {
               own->add_gen_port(cp);
               break;
            }
            case port_o::TLM_IN:
            case port_o::TLM_INOUT:
            case port_o::TLM_OUT:
            case port_o::UNKNOWN:
            default:
               THROW_ERROR(std::string("port type not supported"));
         }
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR(std::string("Signal, port or interface couldn't own a port"));
   }
   return cp;
}

structural_objectRef structural_manager::add_sign_vector(std::string id, unsigned int n_signs, structural_objectRef owner, structural_type_descriptorRef sign_type, unsigned int treenode)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(!check_object(id, owner, signal_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef cs = structural_objectRef(new signal_o(debug_level, owner, signal_vector_o_K));
   cs->set_id(id);
   cs->set_treenode(treenode);
   cs->set_type(sign_type);
   if(n_signs != signal_o::PARAMETRIC_SIGNAL)
      GetPointer<signal_o>(cs)->add_n_signals(n_signs, cs);
   switch(owner->get_kind())
   {
      case component_o_K:
      {
         auto* own = GetPointer<module>(owner);
         own->add_internal_object(cs);
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR(std::string("Only a module can own a signal vector"));
   }
   return cs;
}

structural_objectRef structural_manager::add_sign(std::string id, structural_objectRef owner, structural_type_descriptorRef sign_type, unsigned int treenode)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(!check_object(id, owner, signal_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef cs = structural_objectRef(new signal_o(debug_level, owner, signal_o_K));
   cs->set_id(id);
   cs->set_treenode(treenode);
   cs->set_type(sign_type);
   switch(owner->get_kind())
   {
      case channel_o_K:
      case component_o_K:
      {
         auto* own = GetPointer<module>(owner);
         own->add_internal_object(cs);
         break;
      }
      case bus_connection_o_K:
      {
         auto* own = GetPointer<bus_connection_o>(owner);
         own->add_connection(cs);
         break;
      }
      case action_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR(std::string("Signal, port or interface couldn't own signal"));
   }
   return cs;
}

void structural_manager::remove_empty_signal(structural_objectRef& signal)
{
   auto* sign(GetPointer<signal_o>(signal));
   THROW_ASSERT(sign, "Null argument or not a signal");

   if(sign->get_connected_objects_size() > 0)
   {
      THROW_ERROR("Signal has connected objects");
   }

   structural_objectRef owner(sign->get_owner());

   switch(owner->get_kind())
   {
      case channel_o_K:
      case component_o_K:
         GetPointer<module>(owner)->remove_internal_object(signal);
         break;
      case action_o_K:
      case bus_connection_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Signal removal not implemented for this owner type");
   }
}

void structural_manager::reconnect_signal_member(structural_objectRef& member, structural_objectRef& from_signal, structural_objectRef& to_signal)
{
   // sanity checks

   auto* from(GetPointer<signal_o>(from_signal));
   auto* to(GetPointer<signal_o>(to_signal));
   THROW_ASSERT(from && to && from->get_owner() == to->get_owner(), "Need two valid signals with the same owner");
   THROW_ASSERT(from->is_connected(member), "Not a member of from_signal");
   THROW_ASSERT(check_type(member, to_signal), "Member is incompatible with to_signal");
   THROW_ASSERT(!to->is_connected(member), "Already a member of to_signal");

   // ok, let's rock

   PRINT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "moving member " + member->get_path() + " from " + from_signal->get_path() + " to " + to_signal->get_path());

   switch(member->get_kind())
   {
      case port_o_K:
      {
         auto* p(GetPointer<port_o>(member));
         from->remove_port(member);
         p->remove_connection(from_signal);

         p->add_connection(to_signal);
         to->add_port(member);
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case component_o_K:
      case constant_o_K:
      case data_o_K:
      case event_o_K:
      case port_vector_o_K:
      case signal_o_K:
      case signal_vector_o_K:
      default:
         THROW_ERROR("Unsupported signal member type -- " + std::string(member->get_kind_text()));
   }
}

structural_objectRef structural_manager::add_constant(std::string id, structural_objectRef owner, structural_type_descriptorRef type, std::string value, unsigned int treenode)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(!check_object(id, owner, constant_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef c_obj(new constant_o(debug_level, owner, value));
   c_obj->set_id(id);
   c_obj->set_treenode(treenode);
   c_obj->set_type(type);
   auto* own = GetPointer<module>(owner);
   THROW_ASSERT(own, "Signal, port or interface couldn't own internal object");
   own->add_internal_object(c_obj);
   return c_obj;
}

#if HAVE_TUCANO_BUILT
structural_objectRef structural_manager::add_local_data(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT((owner->get_kind() == component_o_K || owner->get_kind() == channel_o_K), "Only components and channels can have a local_data");
   THROW_ASSERT(!check_object(id, owner, data_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef cs = structural_objectRef(new data_o(debug_level, owner));
   cs->set_id(id);
   cs->set_treenode(treenode);
   cs->set_type(structural_type_descriptorRef(new structural_type_descriptor(treenode, treeM)));
   module* own = GetPointer<module>(owner);
   own->add_local_data(cs);
   return cs;
}

structural_objectRef structural_manager::add_event(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(((owner->get_kind() == component_o_K) || (owner->get_kind() == channel_o_K)), "Only components and channels can have events");
   THROW_ASSERT(!check_object(id, owner, event_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef e = structural_objectRef(new event_o(debug_level, owner));
   e->set_id(id);
   e->set_type(structural_type_descriptorRef(new structural_type_descriptor(treenode, treeM)));
   e->set_treenode(treenode);
   module* own = GetPointer<module>(owner);
   own->add_event(e);
   return e;
}

structural_objectRef structural_manager::add_process(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM, std::string scope, int ft)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(((owner->get_kind() == component_o_K) || (owner->get_kind() == channel_o_K)), "Only components and channels can have processes");
   THROW_ASSERT(!check_object(id, owner, action_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef p = structural_objectRef(new action_o(debug_level, owner));
   p->set_id(id);
   p->set_type(structural_type_descriptorRef(new structural_type_descriptor(treenode, treeM)));
   p->set_treenode(treenode);
   action_o* ad = GetPointer<action_o>(p);
   ad->set_action_type(action_o::process_type(ft));
   ad->set_scope(scope);
   module* own = GetPointer<module>(owner);
   own->add_process(p);
   return p;
}

structural_objectRef structural_manager::add_service(std::string id, std::string interface, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM, std::string scope)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(((owner->get_kind() == component_o_K) || (owner->get_kind() == channel_o_K)), "Only components and channels can have services");
   THROW_ASSERT(!check_object(id, owner, action_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef s = structural_objectRef(new action_o(debug_level, owner));
   s->set_id(id);
   s->set_type(structural_type_descriptorRef(new structural_type_descriptor(treenode, treeM)));
   s->set_treenode(treenode);
   action_o* ad = GetPointer<action_o>(s);
   ad->set_action_type(action_o::SERVICE);
   ad->set_scope(scope);
   module* own = GetPointer<module>(owner);
   own->add_service(s);
   if(owner->get_kind() == channel_o_K)
   {
      channel_o* own_ch = GetPointer<channel_o>(owner);
      own_ch->add_interface(treenode, interface);
   }
   return s;
}

structural_objectRef structural_manager::add_process_param(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(!check_object(id, owner, data_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef data = structural_objectRef(new data_o(debug_level, owner));
   data->set_id(id);
   data->set_type(structural_type_descriptorRef(new structural_type_descriptor(treenode, treeM)));
   data->set_treenode(treenode);
   action_o* ad = GetPointer<action_o>(owner);
   ad->add_parameter(data);
   return data;
}

structural_objectRef structural_manager::add_service_param(std::string id, structural_objectRef owner, unsigned int treenode, const tree_managerRef& treeM)
{
   THROW_ASSERT(owner, "missing the owner");
   THROW_ASSERT(!check_object(id, owner, data_o_K), "the object " + id + " is already present in " + owner->get_path());
   structural_objectRef data = structural_objectRef(new data_o(debug_level, owner));
   data->set_id(id);
   data->set_type(structural_type_descriptorRef(new structural_type_descriptor(treenode, treeM)));
   data->set_treenode(treenode);
   action_o* ad = GetPointer<action_o>(owner);
   ad->add_parameter(data);
   return data;
}
#endif

void structural_manager::add_NP_functionality(structural_objectRef cir, NP_functionality::NP_functionaly_type dt, std::string functionality_description)
{
   THROW_ASSERT((cir->get_kind() == component_o_K), "Only components can have a Non SystemC functionality");
   auto* com = GetPointer<module>(cir);
   NP_functionalityRef f = (com->get_NP_functionality() ? com->get_NP_functionality() : NP_functionalityRef(new NP_functionality));
   f->add_NP_functionality(dt, functionality_description);
   com->set_NP_functionality(f);
}

void structural_manager::SetParameter(const std::string& name, const std::string& value)
{
   THROW_ASSERT((get_circ()->get_kind() == component_o_K), "Only components can have a Non SystemC functionality");
   module* com = GetPointer<module>(get_circ());
   com->SetParameter(name, value);
}

void structural_manager::add_sensitivity(structural_objectRef obj, structural_objectRef pr)
{
   auto* ad = GetPointer<action_o>(pr);
   ad->add_event_to_sensitivity(obj);
}

void structural_manager::add_connection(structural_objectRef src, structural_objectRef dest)
{
   THROW_ASSERT((src && dest), "Missing src or dest: " + (src ? src->get_path() : std::string("!src")) + " " + (dest ? dest->get_path() : std::string("!dest")));
   // std::cerr <<"Adding connection from " + src->get_path() + ":" + src->get_kind_text() + " to "+ dest->get_path() + ":" + dest->get_kind_text() << std::endl;
   switch(src->get_kind())
   {
      case port_o_K:
      {
         auto* p_s = GetPointer<port_o>(src);
         switch(dest->get_kind())
         {
            case port_o_K:
            {
               THROW_ASSERT(src->get_owner() != dest->get_owner(), "A direct connection between ports of the same object is not allowed. Put a signal in between...: " + std::string(src->get_path()) + std::string(" ") + std::string(dest->get_path()));
               THROW_ASSERT(check_type(src, dest), "Ports have to be compatible: " + src->get_path() + " -> " + dest->get_path());
               THROW_ASSERT(!src->find_member(dest->get_id(), port_o_K, dest->get_owner()), "Port " + src->get_id() + " already bound to " + dest->get_id());
               THROW_ASSERT(!dest->find_member(src->get_id(), port_o_K, src->get_owner()), "Port " + dest->get_id() + " already bound to " + src->get_id());
               auto* p_d = GetPointer<port_o>(dest);
               p_s->add_connection(dest);
               p_d->add_connection(src);
               break;
            }
            case signal_o_K:
            {
               THROW_ASSERT(check_type(src, dest), "Ports and signals have to be compatible: " + src->get_path() + " -> " + dest->get_path());
               THROW_ASSERT(!src->find_member(dest->get_id(), signal_o_K, dest->get_owner()), "Port " + src->get_id() + " already bound to " + dest->get_id());
               THROW_ASSERT(!dest->find_member(src->get_id(), port_o_K, src->get_owner()), "Signal " + dest->get_id() + " already bound to " + src->get_id());
               p_s->add_connection(dest);
               auto* c_d = GetPointer<signal_o>(dest);
               c_d->add_port(src);
               break;
            }
            case channel_o_K:
            {
               THROW_ASSERT(src->get_owner() != dest->get_owner(), "A direct connection between port and channel of the same object is not allowed.");
               THROW_ASSERT(!src->find_member(dest->get_id(), channel_o_K, dest->get_owner()), "Port " + src->get_id() + " already bound to " + dest->get_id());
               /// the other check is not currently implemented.
               p_s->add_connection(dest);
               auto* c_d = GetPointer<channel_o>(dest);
               c_d->add_port(src);
               break;
            }
            case constant_o_K:
            {
               // THROW_ASSERT(src->get_owner() != dest->get_owner(), "A direct connection between port and constant of the same object is not allowed.");
               THROW_ASSERT(!src->find_member(dest->get_id(), constant_o_K, dest->get_owner()), "Port " + src->get_id() + " already bound to " + dest->get_id());
               /// the other check is not currently implemented.
               p_s->add_connection(dest);
               auto* c_d = GetPointer<constant_o>(dest);
               c_d->add_connection(src);
               break;
            }
            case action_o_K:
            case bus_connection_o_K:
            case component_o_K:
            case data_o_K:
            case event_o_K:
            case port_vector_o_K:
            case signal_vector_o_K:
            default:
            {
               THROW_ERROR(std::string("Cannot connect a port_o to a ") + dest->get_kind_text() + " " + (src ? src->get_path() : "") + " " + (dest ? dest->get_path() : ""));
            }
         }
         break; // case src = port_o_K
      }
      case port_vector_o_K:
      {
         auto* p_s = GetPointer<port_o>(src);
         switch(dest->get_kind())
         {
            case port_vector_o_K:
            {
               THROW_ASSERT(src->get_owner() != dest->get_owner(), "A direct connection between ports of the same object is not allowed. Put a signal in between...: " + std::string(src->get_path()) + std::string(" ") + std::string(dest->get_path()));
               THROW_ASSERT(check_type(src, dest), "Ports have to be compatible: " + src->get_path() + " -> " + dest->get_path());
               THROW_ASSERT(!src->find_member(dest->get_id(), port_o_K, dest->get_owner()), "Port " + src->get_id() + " already bound to " + dest->get_id());
               THROW_ASSERT(!dest->find_member(src->get_id(), port_o_K, src->get_owner()), "Port " + dest->get_id() + " already bound to " + src->get_id());
               auto* p_d = GetPointer<port_o>(dest);
               p_s->add_connection(dest);
               p_d->add_connection(src);
               break;
            }
            case signal_vector_o_K:
            {
               THROW_ASSERT(check_type(src, dest), "Ports and signals have to be compatible: " + src->get_path() + " -> " + dest->get_path());
               THROW_ASSERT(!src->find_member(dest->get_id(), signal_o_K, dest->get_owner()), "Port " + src->get_id() + " already bound to " + dest->get_id());
               THROW_ASSERT(!dest->find_member(src->get_id(), port_o_K, src->get_owner()), "Signal " + dest->get_id() + " already bound to " + src->get_id());
               p_s->add_connection(dest);
               auto* c_d = GetPointer<signal_o>(dest);
               c_d->add_port(src);
               break;
            }
            case action_o_K:
            case bus_connection_o_K:
            case channel_o_K:
            case component_o_K:
            case constant_o_K:
            case port_o_K:
            case signal_o_K:
            case data_o_K:
            case event_o_K:
            default:
            {
               THROW_ERROR(std::string("Cannot connect a port_vector_o (" + src->get_path() + ") to a ") + dest->get_kind_text() + " (" + dest->get_path() + ")");
            }
         }
         break;
      }
      case signal_o_K:
      {
         auto* s_s = GetPointer<signal_o>(src);
         switch(dest->get_kind())
         {
            case port_o_K:
            {
               THROW_ASSERT(check_type(src, dest), "Ports and signals have to be compatible: " + src->get_path() + " (" + src->get_typeRef()->get_name() + ") -> " + dest->get_path() + " (" + dest->get_typeRef()->get_name() + ")");
               if(src->find_member(dest->get_id(), port_o_K, dest->get_owner()))
               {
                  THROW_WARNING("Signal " + src->get_path() + " already bound to " + dest->get_path());
                  return;
               }
               if(dest->find_member(src->get_id(), signal_o_K, src->get_owner()))
               {
                  THROW_WARNING("Port " + src->get_id() + " already bound to " + dest->get_id());
                  return;
               }
               s_s->add_port(dest);
               auto* p_d = GetPointer<port_o>(dest);
               p_d->add_connection(src);
               break;
            }
            case action_o_K:
            case bus_connection_o_K:
            case channel_o_K:
            case component_o_K:
            case constant_o_K:
            case data_o_K:
            case event_o_K:
            case port_vector_o_K:
            case signal_o_K:
            case signal_vector_o_K:
            default:
            {
               THROW_ERROR(std::string("Cannot connect a signal_o - ") + src->get_path() + std::string(" to a ") + dest->get_kind_text() + std::string(" - ") + dest->get_path());
            }
         }
         break; // case src = signal_o_K
      }
      case signal_vector_o_K:
      {
         auto* s_s = GetPointer<signal_o>(src);
         switch(dest->get_kind())
         {
            case port_vector_o_K:
            {
               THROW_ASSERT(check_type(src, dest), "Ports and signals have to be compatible: " + src->get_path() + " -> " + dest->get_path());
               if(src->find_member(dest->get_id(), port_vector_o_K, dest->get_owner()))
               {
                  THROW_WARNING("Signal " + src->get_id() + " already bound to " + dest->get_id());
                  return;
               }
               if(dest->find_member(src->get_id(), signal_vector_o_K, src->get_owner()))
               {
                  THROW_WARNING("Port " + src->get_id() + " already bound to " + dest->get_id());
                  return;
               }
               s_s->add_port(dest);
               auto* p_d = GetPointer<port_o>(dest);
               p_d->add_connection(src);
               break;
            }
            case action_o_K:
            case bus_connection_o_K:
            case channel_o_K:
            case component_o_K:
            case constant_o_K:
            case data_o_K:
            case event_o_K:
            case port_o_K:
            case signal_o_K:
            case signal_vector_o_K:
            default:
            {
               THROW_ERROR(std::string("Cannot connect a signal_vector_o - ") + src->get_path() + std::string(" to a ") + dest->get_kind_text() + std::string(" - ") + dest->get_path());
            }
         }
         break;
      }
      case constant_o_K:
      {
         auto* c_s = GetPointer<constant_o>(src);
         switch(dest->get_kind())
         {
            case port_o_K:
            {
               THROW_ASSERT(check_type(src, dest), "Incompatible object types: " + src->get_id() + " -> " + dest->get_id());
               // THROW_ASSERT(src->get_owner() != dest->get_owner(), "A direct connection between a constant and a port of the same object is not allowed.");
               THROW_ASSERT(!dest->find_member(src->get_id(), constant_o_K, src->get_owner()), "Port " + dest->get_id() + " already bound to " + src->get_id());
               c_s->add_connection(dest);
               auto* p_d = GetPointer<port_o>(dest);
               p_d->add_connection(src);
               break;
            }
            case action_o_K:
            case bus_connection_o_K:
            case channel_o_K:
            case component_o_K:
            case constant_o_K:
            case data_o_K:
            case event_o_K:
            case port_vector_o_K:
            case signal_o_K:
            case signal_vector_o_K:
            default:
            {
               THROW_ERROR(std::string("Cannot connect a constant_o to a ") + dest->get_kind_text());
            }
         }
         break; // case src = constant_o_K
      }
      case action_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case component_o_K:
      case data_o_K:
      case event_o_K:
      default:
      {
         THROW_ERROR(std::string("Cannot connect a ") + src->get_kind_text() + " to a " + dest->get_kind_text());
      }
   } // switch src kind
}

void structural_manager::WriteDot(const std::string& file_name, circuit_graph_type gt, graph* g) const
{
   const std::string output_directory = Param->getOption<std::string>(OPT_dot_directory);
   std::ofstream f((output_directory + file_name).c_str());

   if(g == nullptr)
   {
      switch(gt)
      {
         case DATA_G:
            g = data_graph;
            break;
         case COMPLETE_G:
            g = circuit_graph;
            break;
         default:
            THROW_ERROR(std::string("Not supported graph type"));
      }
   }
   boost::write_graphviz(f, *g, cg_label_writer(g), cg_edge_writer(g));
}

void structural_manager::print(std::ostream& os) const
{
   if(circuit && circuit->get_id() != "")
   {
      auto* circ = GetPointer<component_o>(circuit);
      circ->print(os);
   }
   else
      os << "Error : empty circuit.\n";
}

const vertex& structural_manager::get_PI(const structural_objectRef) const
{
   auto* gi = GetPointer<cg_graph_info>(circuit_graph->GetGraphInfo());
   THROW_ASSERT(gi, "graph property not associated with the circuit graph");
   return gi->Entry;
}

void structural_manager::check_structure(structural_objectRef obj, bool permissive)
{
   auto* top_c = GetPointer<module>(obj);
   if(top_c->get_internal_objects_size() == 0 && obj->get_owner()) // check has mean for all component but the top
   {
      for(unsigned int i = 0; i < top_c->get_in_port_size(); i++)
      {
         auto* temp = GetPointer<port_o>(top_c->get_in_port(i));
         if(!temp)
            THROW_ERROR("not expected type of port");
         if(temp->get_kind() == port_o_K && temp->get_connections_size() == 0 && !permissive)
            THROW_ERROR(std::string("Error : component " + obj->get_id() + " input port " + temp->get_id() + " is not bound\n"));
         else if(temp->get_kind() == port_o_K && temp->get_connections_size() == 0 && permissive)
         {
            if(debug_level >= DEBUG_LEVEL_VERBOSE)
               THROW_WARNING("component " + obj->get_id() + " input port " + temp->get_id() + " is not bound");
         }
         else if(temp->get_kind() == port_vector_o_K)
         {
            for(unsigned int w = 0; w < temp->get_ports_size(); w++)
            {
               auto* tempi = GetPointer<port_o>(temp->get_port(w));
               THROW_ASSERT(tempi, "Expected a port got something of different");
               if(tempi->get_connections_size() == 0 && !permissive)
                  THROW_ERROR(std::string("Component " + obj->get_id() + " input port " + tempi->get_id() + " is not bound\n"));
               else if(tempi && tempi->get_connections_size() == 0 && permissive)
               {
                  if(debug_level >= DEBUG_LEVEL_VERBOSE)
                     THROW_WARNING("component " + obj->get_id() + " input port " + tempi->get_id() + " is not bound");
               }
            }
         }
      }
      for(unsigned int i = 0; i < top_c->get_out_port_size(); i++)
      {
         auto* temp = GetPointer<port_o>(top_c->get_out_port(i));
         if(!temp)
            THROW_ERROR("not expected type of port");
         if(temp->get_kind() == port_o_K && temp->get_connections_size() == 0 && !permissive)
            THROW_ERROR(std::string("Error : component " + obj->get_id() + " output port " + temp->get_id() + " is not bound\n"));
         else if(temp->get_kind() == port_o_K && temp->get_connections_size() == 0 && permissive)
         {
            if(debug_level >= DEBUG_LEVEL_VERBOSE)
               THROW_WARNING("component " + obj->get_id() + " output port " + temp->get_id() + " is not bound");
         }
         else if(temp->get_kind() == port_vector_o_K)
         {
            for(unsigned int w = 0; w < temp->get_ports_size(); w++)
            {
               auto* tempi = GetPointer<port_o>(temp->get_port(w));
               THROW_ASSERT(tempi, "Expected a port got something of different");
               if(tempi->get_connections_size() == 0 && !permissive)
                  THROW_ERROR(std::string("Error : component " + obj->get_id() + " output port " + tempi->get_id() + " is not bound\n"));
               else if(tempi->get_connections_size() == 0 && permissive)
               {
                  if(debug_level >= DEBUG_LEVEL_VERBOSE)
                     THROW_WARNING("component " + obj->get_id() + " output port " + tempi->get_id() + " is not bound");
               }
            }
         }
      }
      for(unsigned int i = 0; i < top_c->get_in_out_port_size(); i++)
      {
         auto* temp = GetPointer<port_o>(top_c->get_in_out_port(i));
         if(!temp)
            THROW_ERROR("not expected type of port");
         if(temp->get_kind() == port_o_K && temp->get_connections_size() == 0 && !permissive)
            THROW_ERROR(std::string("Error : component " + obj->get_id() + " in/output port " + temp->get_id() + " is not bound\n"));
         else if(temp->get_kind() == port_o_K && temp->get_connections_size() == 0 && permissive)
         {
            if(debug_level >= DEBUG_LEVEL_VERBOSE)
               THROW_WARNING("component " + obj->get_id() + " in/output port " + temp->get_id() + " is not bound");
         }
         else if(temp->get_kind() == port_vector_o_K)
         {
            for(unsigned int w = 0; w < temp->get_ports_size(); w++)
            {
               auto* tempi = GetPointer<port_o>(temp->get_port(w));
               THROW_ASSERT(tempi, "Expected a port got something of different");
               if(tempi->get_connections_size() == 0 && !permissive)
                  THROW_ERROR(std::string("Error : component " + obj->get_id() + " in/output port " + tempi->get_id() + " is not bound\n"));
               else if(tempi->get_connections_size() == 0 && permissive)
               {
                  if(debug_level >= DEBUG_LEVEL_VERBOSE)
                     THROW_WARNING("component " + obj->get_id() + " in/output port " + tempi->get_id() + " is not bound");
               }
            }
         }
      }
      for(unsigned int i = 0; i < top_c->get_gen_port_size(); i++)
      {
         auto* temp = GetPointer<port_o>(top_c->get_gen_port(i));
         if(!temp)
            THROW_ERROR("not expected type of port");
         if(temp->get_kind() == port_o_K && temp->get_connections_size() == 0 && !permissive)
            THROW_ERROR(std::string("Error : component " + obj->get_id() + " generic port " + temp->get_id() + " is not bound\n"));
         else if(temp->get_kind() == port_o_K && temp->get_connections_size() == 0 && permissive)
         {
            if(debug_level >= DEBUG_LEVEL_VERBOSE)
               THROW_WARNING("component " + obj->get_id() + " generic port " + temp->get_id() + " is not bound");
         }
         else if(temp->get_kind() == port_vector_o_K)
         {
            for(unsigned int w = 0; w < temp->get_ports_size(); w++)
            {
               auto* tempi = GetPointer<port_o>(temp->get_port(w));
               THROW_ASSERT(tempi, "Expected a port got something of different");
               if(tempi->get_connections_size() == 0 && !permissive)
                  THROW_ERROR(std::string("Error : component " + obj->get_id() + " generic port " + tempi->get_id() + " is not bound\n"));
               else if(tempi->get_connections_size() == 0 && permissive)
               {
                  if(debug_level >= DEBUG_LEVEL_VERBOSE)
                     THROW_WARNING("component " + obj->get_id() + " generic port " + tempi->get_id() + " is not bound");
               }
            }
         }
      }
   }
   for(unsigned int i = 0; i < top_c->get_internal_objects_size(); i++)
   {
      switch(top_c->get_internal_object(i)->get_kind())
      {
         case channel_o_K:
         case component_o_K:
         {
            if(!top_c->get_internal_object(i)->get_black_box())
               check_structure(top_c->get_internal_object(i), permissive);
            break;
         }
         case constant_o_K:
         case signal_o_K:
         case signal_vector_o_K:
         case bus_connection_o_K:
            break;
         case action_o_K:
         case data_o_K:
         case event_o_K:
         case port_o_K:
         case port_vector_o_K:
         default:
            THROW_ERROR("Internal object not foreseen: " + std::string(top_c->get_internal_object(i)->get_kind_text()));
      }
   }
}

void structural_manager::INIT(bool permissive)
{
   check_structure(circuit, permissive);
   if(og)
      delete og;
   if(data_graph)
      delete data_graph;
   if(circuit_graph)
      delete circuit_graph;
   og = new graphs_collection(GraphInfoRef(new cg_graph_info), Param);
   data_graph = new graph(og, PURE_DATA_SELECTOR);
   circuit_graph = new graph(og, ALL_LINES_SELECTOR);
   build_graph(circuit, og);
}

#if HAVE_BAMBU_BUILT || HAVE_KOALA_BUILT || HAVE_EUCALYPTUS_BUILT
structural_objectRef structural_manager::add_module_from_technology_library(const std::string& id, const std::string& fu_name, const std::string& library_name, const structural_objectRef owner, const technology_managerConstRef TM)
{
   THROW_ASSERT(!owner || owner->get_kind() == component_o_K || owner->get_kind() == channel_o_K, "Expected a component or a channel got something of different");
   technology_nodeRef port_tec_node = TM->get_fu(fu_name, library_name);
   THROW_ASSERT(port_tec_node, std::string("gate not found: ") + fu_name + " " + std::string(" in library: ") + library_name);
   if(GetPointer<functional_unit_template>(port_tec_node))
      port_tec_node = GetPointer<functional_unit_template>(port_tec_node)->FU;
   THROW_ASSERT(port_tec_node, "port_tec_node is null");
   THROW_ASSERT(GetPointer<functional_unit>(port_tec_node), "GetPointer<functional_unit>(port_tec_node) is null");
   THROW_ASSERT(GetPointer<functional_unit>(port_tec_node)->CM, "GetPointer<functional_unit>(port_tec_node)->CM is null for " + fu_name);
   structural_objectRef curr_lib_instance = GetPointer<functional_unit>(port_tec_node)->CM->get_circ();
   THROW_ASSERT(curr_lib_instance->get_kind() == component_o_K, "Expected a component got something of different");
   structural_objectRef curr_gate = structural_objectRef(new component_o(debug_level, owner));
   if(!owner)
      circuit = curr_gate; // set the top component of the circuit manager.
   curr_lib_instance->copy(curr_gate);
   curr_gate->set_id(id);
   if(owner)
      GetPointer<module>(owner)->add_internal_object(curr_gate);
   return curr_gate;
}
#endif

/**
 * this template function adds an edge to the bulk graph and possibly a label to the edge. Parallel edges are allowed.
 */
template <class Graph>
void circuit_add_edge(typename boost::graph_traits<Graph>::vertex_descriptor A, typename boost::graph_traits<Graph>::vertex_descriptor B, int selector, Graph& g, const structural_objectRef from1, structural_objectRef to1, bool is_critical = false)
{
   typename boost::graph_traits<Graph>::out_edge_iterator oi, oi_end;
   typename boost::graph_traits<Graph>::edge_descriptor e;
   bool inserted;
   for(boost::tie(oi, oi_end) = boost::out_edges(A, g); oi != oi_end; oi++)
   {
      if(boost::target(*oi, g) != B)
         continue;
      if(GET_FROM_PORT(&g, *oi) == from1 && GET_TO_PORT(&g, *oi) == to1)
         return;
   }
   boost::tie(e, inserted) = boost::add_edge(A, B, EdgeProperty(selector), g);
   EDGE_ADD_FROM_PORT(&g, e, from1);
   EDGE_ADD_TO_PORT(&g, e, to1);
   EDGE_SET_CRITICAL(&g, e, is_critical);
}

/**
 * Add a directed edge between the nodes associated with p1 and p2.
 * @param module_vertex_rel is the relation between modules and vertexes.
 * @param p1 is the first port.
 * @param p2 is the second port.
 * @param en is the entry vertex.
 * @param ex is the exit vertex.
 */
static void add_directed_edge_single(graphs_collection* bg, const std::map<structural_objectRef, boost::graph_traits<graphs_collection>::vertex_descriptor>& module_vertex_rel, const structural_objectRef& p1, const structural_objectRef& p2,
                                     boost::graph_traits<graphs_collection>::vertex_descriptor en, boost::graph_traits<graphs_collection>::vertex_descriptor ex, bool is_critical = false)
{
   THROW_ASSERT(p1->get_kind() == port_o_K or p1->get_kind() == port_vector_o_K, "Expected a port got something of different");
   THROW_ASSERT(p2->get_kind() == port_o_K or p2->get_kind() == port_vector_o_K, "Expected a port got something of different");

   structural_objectRef p_obj1 = p1;
   structural_objectRef p_obj2 = p2;

   // now detect the vertex associated with the ports
   boost::graph_traits<graphs_collection>::vertex_descriptor src, tgt = boost::graph_traits<graphs_collection>::null_vertex();

   structural_objectRef owner1 = p_obj1->get_owner();
   structural_objectRef owner2 = p_obj2->get_owner();

   // first detect the actual owner
   if(owner1 && owner1->get_kind() == port_vector_o_K)
      owner1 = owner1->get_owner();
   if(owner2 && owner2->get_kind() == port_vector_o_K)
      owner2 = owner2->get_owner();

   int edge_type = DATA_SELECTOR;
   if(GetPointer<port_o>(p_obj2)->get_is_clock() || GetPointer<port_o>(p_obj2)->get_is_clock())
      edge_type = CLOCK_SELECTOR;

   if(owner1 == owner2) // pass through signal
   {
      if(GetPointer<port_o>(p_obj1)->get_port_direction() == port_o::OUT and GetPointer<port_o>(p_obj2)->get_port_direction() == port_o::IN)
      {
         std::swap(p_obj1, p_obj2);
      }

      src = en;
      tgt = ex;
   }
   else
   {
      /// p2 is a top port
      if(owner1 and owner1->get_owner() == owner2)
      {
         src = module_vertex_rel.find(owner1)->second;
         if(GetPointer<port_o>(p_obj2)->get_port_direction() == port_o::OUT)
            tgt = ex;
         else if(GetPointer<port_o>(p_obj2)->get_port_direction() == port_o::IN)
            tgt = en;
         else if(GetPointer<port_o>(p_obj2)->get_port_direction() == port_o::IO)
         {
            if(GetPointer<port_o>(p_obj1)->get_port_direction() == port_o::IN)
               tgt = en;
            else
               tgt = ex;
         }
         else
            THROW_ERROR("Something wrong");
         if(GetPointer<port_o>(p_obj1)->get_port_direction() == port_o::IN)
         {
            std::swap(src, tgt);
            std::swap(p_obj1, p_obj2);
         }
      }
      /// p1 is a top port
      else if(owner2 and owner2->get_owner() == owner1)
      {
         src = en;
         tgt = module_vertex_rel.find(owner2)->second;
      }
      else // module port
      {
         src = module_vertex_rel.find(owner1)->second;
         tgt = module_vertex_rel.find(owner2)->second;

         if(GetPointer<port_o>(p_obj1)->get_port_direction() == port_o::IN and GetPointer<port_o>(p_obj2)->get_port_direction() == port_o::OUT)
         {
            std::swap(src, tgt);
            std::swap(p_obj1, p_obj2);
         }

         /// hyper-edge and not significant connectivity
         if(GetPointer<port_o>(p_obj1)->get_port_direction() == port_o::IN and GetPointer<port_o>(p_obj2)->get_port_direction() == port_o::IN)
         {
            // std::cerr << "hyper-edge and not significant connectivity" << std::endl;
            return;
         }

         THROW_ASSERT(GetPointer<port_o>(p_obj1)->get_port_direction() == port_o::OUT and GetPointer<port_o>(p_obj2)->get_port_direction() == port_o::IN, "Not supported situation");
      }
   }

   circuit_add_edge(src, tgt, edge_type, *bg, p_obj1, p_obj2, is_critical);

#if 0
   //try to detect a direction
   if ((pp1->get_port_direction() == port_o::IN && pp2->get_port_direction() != port_o::IN) ||
       (pp1->get_port_direction() != port_o::OUT && pp2->get_port_direction() == port_o::OUT)
       )
      std::swap(pp1, pp2);

   else
   {
      if (owner2 && owner1 == owner2->get_owner())// pp1 is a top port
      {
         THROW_ASSERT(module_vertex_rel.find(owner2) != module_vertex_rel.end(), "module not found");
         v2 = module_vertex_rel.find(owner2)->second;
         if (pp1->get_port_direction() != port_o::OUT or pp2->get_port_direction() == port_o::IN)
            v1 = en;
         else
         {
            v1 = ex;
            std::swap(v1, v2);
            std::swap(pp1, pp2);
         }
      }
      else if (owner1 && owner2 == owner1->get_owner())// pp2 is a top port
      {
         THROW_ASSERT(module_vertex_rel.find(owner1) != module_vertex_rel.end(), "module not found");
         v1 = module_vertex_rel.find(owner1)->second;
         if (pp2->get_port_direction() != port_o::OUT or pp1->get_port_direction() == port_o::IN)
         {
            v2 = en;
            std::swap(v1, v2);
            std::swap(pp1, pp2);
         }
         else
            v2 = ex;
      }
      else
      {
         THROW_ASSERT(module_vertex_rel.find(owner1) != module_vertex_rel.end(), "module not found");
         v1 = module_vertex_rel.find(owner1)->second;
         THROW_ASSERT(module_vertex_rel.find(owner2) != module_vertex_rel.end(), "module not found");
         v2 = module_vertex_rel.find(owner2)->second;
      }
   }
   //std::cerr << "     connection " << pp1->get_path() << " -> " << pp2->get_path() << std::endl;
   //in case both ports are IO or GEN two opposite edges are added
   if ((pp1->get_port_direction() == port_o::IO || pp1->get_port_direction() == port_o::GEN) && (pp2->get_port_direction() == port_o::IO || pp2->get_port_direction() == port_o::GEN))
   {
      circuit_add_edge(v1, v2, DATA_SELECTOR, *bg, p1, p2, is_critical);
      circuit_add_edge(v2, v1, DATA_SELECTOR, *bg, p2, p1, is_critical);
   }
   else if ((pp1->get_port_direction() == port_o::IN && pp2->get_port_direction() == port_o::IN  && ((v1 == en && v2 != en) || (v1 != en && v2 == en))) ||
            (pp1->get_port_direction() == port_o::OUT && pp2->get_port_direction() == port_o::OUT  && ((v1 == ex && v2 != ex) || (v1 != ex && v2 == ex))) ||
            pp1->get_port_direction() != pp2->get_port_direction()
            )
   {
      structural_objectRef port;
      if (pp1 != GetPointer<port_o>(p1)) //check possible swap
      {
         if (pp1->get_is_clock() || pp2->get_is_clock())
            circuit_add_edge(v1, v2, CLOCK_SELECTOR, *bg, p2, p1, is_critical);
         else
            circuit_add_edge(v1, v2, DATA_SELECTOR, *bg, p2, p1, is_critical);
      }
      else
      {
         if (pp1->get_is_clock() || pp2->get_is_clock())
            circuit_add_edge(v1, v2, CLOCK_SELECTOR, *bg, p1, p2, is_critical);
         else
            circuit_add_edge(v1, v2, DATA_SELECTOR, *bg, p1, p2, is_critical);
      }
   }
#endif
}

/**
 * Add a directed edge between the nodes associated with p1 and p2.
 * @param module_vertex_rel is the relation between modules and vertexes.
 * @param p1 is the first port.
 * @param p2 is the second object. It could be a port, a signal or a module.
 * @param en is the entry vertex.
 * @param ex is the exit vertex.
 */
static void add_directed_edge(graphs_collection* bg, const std::map<structural_objectRef, boost::graph_traits<graphs_collection>::vertex_descriptor>& module_vertex_rel, const structural_objectRef& p1, const structural_objectRef& p2,
                              boost::graph_traits<graphs_collection>::vertex_descriptor en, boost::graph_traits<graphs_collection>::vertex_descriptor ex, bool is_critical = false)
{
   // the port is not connected to any object
   if(!p2)
      return;
   THROW_ASSERT(p1, "not valid object");

   // std::cerr << "p1 is " << p1->get_kind_text() << std::endl;
   // std::cerr << "p2 is " << p2->get_kind_text() << std::endl;

   switch(p2->get_kind())
   {
      case port_o_K:
      case port_vector_o_K:
      {
         // std::cerr << "p1: " << p1->get_path() << "(" << p1->get_kind_text() << ") -> p2: " << p2->get_path() << "(" << p2->get_kind_text() << ")" << std::endl;
         add_directed_edge_single(bg, module_vertex_rel, p1, p2, en, ex, is_critical);
         break;
      }
      case signal_o_K:
      case signal_vector_o_K:
      {
         auto* conn = GetPointer<signal_o>(p2);
         for(unsigned int k = 0; k < conn->get_connected_objects_size(); k++)
         {
            if(conn->get_port(k) != p1)
            {
               // std::cerr << "p1: " << p1->get_path() << "(" << p1->get_kind_text() << ") -> p2: " << conn->get_port(k)->get_path() << "(" << conn->get_port(k)->get_kind_text() << ")" << std::endl;
               add_directed_edge_single(bg, module_vertex_rel, p1, conn->get_port(k), en, ex, is_critical);
            }
         }
         break;
      }
      case constant_o_K:
      {
         structural_objectRef owner1 = p1->get_owner();
         boost::graph_traits<graphs_collection>::vertex_descriptor v1;
         if(owner1 && owner1->get_kind() == port_vector_o_K)
            owner1 = owner1->get_owner();
         THROW_ASSERT(module_vertex_rel.find(owner1) != module_vertex_rel.end(), "module not found");
         v1 = module_vertex_rel.find(owner1)->second;
         circuit_add_edge(en, v1, DATA_SELECTOR, *bg, p2, p1, is_critical);
         break;
      }
      case component_o_K:
      {
         THROW_WARNING("Still to be checked!");
         /// gen port connection
         structural_objectRef owner1 = p1->get_owner();
         structural_objectRef owner2 = p2->get_owner();
         boost::graph_traits<graphs_collection>::vertex_descriptor v1, v2;
         if(owner1 && owner1->get_kind() == port_vector_o_K)
            owner1 = owner1->get_owner();
         if(owner1 == owner2) // top ports
            v1 = en;
         else
         {
            THROW_ASSERT(module_vertex_rel.find(owner1) != module_vertex_rel.end(), "module not found");
            v1 = module_vertex_rel.find(owner1)->second;
         }
         THROW_ASSERT(module_vertex_rel.find(p2) != module_vertex_rel.end(), "module not found");
         v2 = module_vertex_rel.find(p2)->second;
         circuit_add_edge(v1, v2, CHANNEL_SELECTOR, *bg, p1, p2, is_critical);
         break;
      }
      case action_o_K:
      case bus_connection_o_K:
      case channel_o_K:
      case data_o_K:
      case event_o_K:
      default:
      {
         THROW_ERROR("unexpected object type: " + std::string(p2->get_kind_text()));
      }
   }
}

void structural_manager::build_graph(const structural_objectRef& top, graphs_collection* bg)
{
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Building graph of " + top->get_typeRef()->id_type);

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Creating the PI node");
   // create the entry node
   vertex v_en = bg->AddVertex(NodeInfoRef(new cg_node_info()));
   std::string v_en_name = "PI";
   GET_NODE_INFO(bg, cg_node_info, v_en)->vertex_name = v_en_name;
   GET_NODE_INFO(bg, cg_node_info, v_en)->node_type = TYPE_ENTRY;
   GET_NODE_INFO(bg, cg_node_info, v_en)->node_operation = ENTRY;
   const module* mod = GetPointer<module>(top);
   for(unsigned int p = 0; p < mod->get_in_port_size(); p++)
   {
      structural_objectRef port = mod->get_in_port(p);
      if(GetPointer<port_o>(port)->get_critical())
         GET_NODE_INFO(bg, cg_node_info, v_en)->is_critical = true;
   }

   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Creating the PO node");
   // create the exit node
   vertex v_ex = bg->AddVertex(NodeInfoRef(new cg_node_info()));
   std::string v_ex_name = "PO";
   GET_NODE_INFO(bg, cg_node_info, v_ex)->vertex_name = v_ex_name;
   GET_NODE_INFO(bg, cg_node_info, v_ex)->node_type = TYPE_EXIT;
   GET_NODE_INFO(bg, cg_node_info, v_ex)->node_operation = EXIT;
   for(unsigned int p = 0; p < mod->get_out_port_size(); p++)
   {
      structural_objectRef port = mod->get_out_port(p);
      if(GetPointer<port_o>(port)->get_critical())
         GET_NODE_INFO(bg, cg_node_info, v_ex)->is_critical = true;
   }

   // attach the graph property to the graph
   auto* graph_info = GetPointer<cg_graph_info>(circuit_graph->GetGraphInfo());
   graph_info->Entry = v_en;
   graph_info->Entry_name = v_en_name;
   graph_info->Exit = v_ex;
   graph_info->Exit_name = v_ex_name;

   /// add all nodes
   std::map<structural_objectRef, boost::graph_traits<graphs_collection>::vertex_descriptor> module_vertex_rel;
   for(unsigned int i = 0; i < mod->get_internal_objects_size(); i++)
   {
      switch(mod->get_internal_object(i)->get_kind())
      {
         case channel_o_K:
         case component_o_K:
         {
            vertex curr_v = bg->AddVertex(NodeInfoRef(new cg_node_info));
            module_vertex_rel[mod->get_internal_object(i)] = curr_v;
            auto* mod_int = GetPointer<module>(mod->get_internal_object(i));
            GET_NODE_INFO(bg, cg_node_info, curr_v)->vertex_name = mod_int->get_id();
            GET_NODE_INFO(bg, cg_node_info, curr_v)->node_operation = GET_TYPE_NAME(mod_int);
            GET_NODE_INFO(bg, cg_node_info, curr_v)->reference = mod->get_internal_object(i);
            if(mod->get_internal_object(i)->get_kind() == component_o_K)
               GET_NODE_INFO(bg, cg_node_info, curr_v)->is_critical = GetPointer<module>(mod->get_internal_object(i))->get_critical();
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, " - Creating the node for the instance " + mod_int->get_path());
            break;
         }
         case signal_o_K:
         case constant_o_K:
         case signal_vector_o_K:
         case bus_connection_o_K:
            break; /// no action for signals and bus
         case action_o_K:
         case data_o_K:
         case event_o_K:
         case port_o_K:
         case port_vector_o_K:
         default:
            THROW_ERROR("Structural object not foreseen: " + std::string(mod->get_internal_object(i)->get_kind_text()));
      }
   }
   for(unsigned int i = 0; i < mod->get_internal_objects_size(); i++)
   {
      switch(mod->get_internal_object(i)->get_kind())
      {
         case channel_o_K:
         case component_o_K:
         {
            auto* mod_inst = GetPointer<module>(mod->get_internal_object(i));
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Analyzing instance " + mod_inst->get_path());
            if(mod_inst->get_in_port_size())
            {
               // analyzing input ports
               for(unsigned int j = 0; j < mod_inst->get_in_port_size(); j++)
               {
                  const structural_objectRef& in_port = mod_inst->get_in_port(j);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   * Input port: " + in_port->get_path());
                  // the input port is a single port
                  if(in_port->get_kind() == port_o_K)
                  {
                     const port_o* p = GetPointer<port_o>(in_port);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - Single port");
                     const structural_objectRef& bounded = p->find_bounded_object();
                     if(!bounded)
                        continue;
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - Bounded object: " + bounded->get_path() + " - " + bounded->get_kind_text());
                     if(!(GetPointer<signal_o>(bounded) && !(GetPointer<signal_o>(bounded)->is_full_connected())))
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - Adding direct edge from " + bounded->get_path() + " to " + p->get_path());
                        bool is_critical = false;
                        if(p->get_critical())
                        {
                           if((GetPointer<port_o>(bounded)->get_critical()) or (GetPointer<signal_o>(bounded) && GetPointer<signal_o>(bounded)->get_critical()))
                              is_critical = true;
                        }
                        add_directed_edge(bg, module_vertex_rel, in_port, bounded, v_en, v_ex, is_critical);
                     }
                  }
                  else
                  {
                     auto* pv = GetPointer<port_o>(mod_inst->get_in_port(j));
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - Port vector");
                     for(unsigned int k = 0; k < pv->get_ports_size(); k++)
                     {
                        const structural_objectRef& in_port_i = pv->get_port(k);
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "      - Port: " + in_port_i->get_path());
                        const structural_objectRef& bounded = GetPointer<port_o>(pv->get_port(k))->find_bounded_object();
                        if(!bounded)
                           continue;
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "       - Bounded object: " + bounded->get_path() + " - " + bounded->get_kind_text());
                        if(!(GetPointer<signal_o>(bounded) && !(GetPointer<signal_o>(bounded)->is_full_connected())))
                        {
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "       - Adding direct edge from " + bounded->get_path() + " to " + in_port_i->get_path());
                           if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                           {
                              THROW_WARNING("Critical paths are not correctly identified when port vectors are involved");
                           }
                           add_directed_edge(bg, module_vertex_rel, in_port_i, bounded, v_en, v_ex);
                        }
                     }
                  }
               }
            }
            // analyzing output ports
            if(mod_inst->get_out_port_size())
            {
               for(unsigned int j = 0; j < mod_inst->get_out_port_size(); j++)
               {
                  const structural_objectRef& out_port = mod_inst->get_out_port(j);
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Output port: " + out_port->get_path());
                  if(out_port->get_kind() == port_o_K)
                  {
                     const port_o* p = GetPointer<port_o>(out_port);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - Single port");
                     const structural_objectRef& target = p->find_bounded_object();
                     if(!target)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                        continue;
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - Bounded object: " + target->get_path() + " - " + target->get_kind_text());
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "     - Adding direct edge from " + out_port->get_path() + " to " + target->get_path());
                     bool is_critical = false;
                     if(GetPointer<port_o>(out_port)->get_critical())
                     {
                        if(GetPointer<port_o>(target)->get_critical())
                           is_critical = true;
                     }
                     add_directed_edge(bg, module_vertex_rel, out_port, target, v_en, v_ex, is_critical);
                  }
                  else
                  {
                     auto* pv = GetPointer<port_o>(out_port);
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "-->Port vector");
                     const structural_objectRef target_vector = pv->find_bounded_object();
                     if(target_vector)
                     {
                        INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding direct edge from " + pv->get_path() + " to " + target_vector->get_path());
                        add_directed_edge(bg, module_vertex_rel, out_port, target_vector, v_en, v_ex);
                     }
                     else
                     {
                        for(unsigned int k = 0; k < pv->get_ports_size(); k++)
                        {
                           const port_o* p = GetPointer<port_o>(pv->get_port(k));
                           const structural_objectRef& target = p->find_bounded_object();
                           if(!target)
                              continue;
                           INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "---Adding direct edge from " + p->get_path() + " to " + target->get_path());
                           if(debug_level >= DEBUG_LEVEL_VERY_PEDANTIC)
                           {
                              THROW_WARNING("Critical paths are not correctly identified when port vectors are involved");
                           }
                           add_directed_edge(bg, module_vertex_rel, pv->get_port(k), target, v_en, v_ex);
                        }
                     }
                     INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
                  }
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--");
               }
            }
            // analyzing in/out ports
            if(mod_inst->get_in_out_port_size())
            {
               for(unsigned int j = 0; j < mod_inst->get_in_out_port_size(); j++)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   * In/Out port: " + mod_inst->get_in_out_port(j)->get_path());
                  const structural_objectRef& bounded = GetPointer<port_o>(mod_inst->get_in_out_port(j))->find_bounded_object();
                  if(!bounded)
                     continue;
                  if(mod_inst->get_in_out_port(j)->get_kind() == port_o_K)
                  {
                     add_directed_edge(bg, module_vertex_rel, mod_inst->get_in_out_port(j), bounded, v_en, v_ex);
                  }
                  else
                  {
                     auto* pv = GetPointer<port_o>(mod_inst->get_in_out_port(j));
                     for(unsigned int k = 0; k < pv->get_ports_size(); k++)
                     {
                        const structural_objectRef& bounded_k = GetPointer<port_o>(pv->get_port(k))->find_bounded_object();
                        if(!bounded_k)
                           continue;
                        add_directed_edge(bg, module_vertex_rel, pv->get_port(k), bounded_k, v_en, v_ex);
                     }
                  }
               }
            }
            // analyzing generic ports
            if(mod_inst->get_gen_port_size())
            {
               for(unsigned int j = 0; j < mod_inst->get_gen_port_size(); j++)
               {
                  INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "   * Generic port: " + mod_inst->get_gen_port(j)->get_path());
                  const structural_objectRef& bounded = GetPointer<port_o>(mod_inst->get_gen_port(j))->find_bounded_object();
                  if(!bounded)
                     continue;
                  if(mod_inst->get_gen_port(j)->get_kind() == port_o_K)
                  {
                     add_directed_edge(bg, module_vertex_rel, mod_inst->get_gen_port(j), bounded, v_en, v_ex);
                  }
                  else
                  {
                     auto* pv = GetPointer<port_o>(mod_inst->get_gen_port(j));
                     for(unsigned int k = 0; k < pv->get_ports_size(); k++)
                     {
                        const structural_objectRef& bounded_k = GetPointer<port_o>(pv->get_port(k))->find_bounded_object();
                        if(!bounded_k)
                           continue;
                        add_directed_edge(bg, module_vertex_rel, pv->get_port(k), bounded_k, v_en, v_ex);
                     }
                  }
               }
            }
            INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Analyzed instance " + mod_inst->get_path());
            break;
         }
         case constant_o_K:
         case signal_o_K:
         case signal_vector_o_K:
         case bus_connection_o_K:
            break; /// no action for signals and bus
         case action_o_K:
         case data_o_K:
         case event_o_K:
         case port_o_K:
         case port_vector_o_K:
         default:
            THROW_ERROR("Structural object not foreseen: " + std::string(mod->get_internal_object(i)->get_kind_text()));
      }
   }
   INDENT_DBG_MEX(DEBUG_LEVEL_VERY_PEDANTIC, debug_level, "<--Built graph of " + top->get_typeRef()->id_type);
}

void structural_manager::xload(const xml_element* node, structural_managerRef const& CM)
{
   // Recurse through child nodes:
   const xml_node::node_list list = node->get_children();
   for(const auto& iter : list)
   {
      const auto* Enode = GetPointer<const xml_element>(iter);
      if(!Enode || Enode->get_name() != GET_CLASS_NAME(component_o))
         continue;
      CM->get_circ()->xload(Enode, CM->get_circ(), CM);
   }
}

void structural_manager::xwrite(xml_element* rootnode, const technology_nodeRef&
#if HAVE_KOALA_BUILT
                                                           tn
#endif
                                ) const
{
#if HAVE_KOALA_BUILT
   get_circ()->xwrite_attributes(rootnode, tn);
#endif

   xml_element* CMnode = rootnode->add_child_element("circuit");
   get_circ()->xwrite(CMnode);
}

void remove_port_connection(const structural_objectRef& obj)
{
   auto* pin = GetPointer<port_o>(obj);
   for(unsigned int j = 0; j < pin->get_connections_size(); j++)
   {
      structural_objectRef comp = pin->get_connection(j);
      if(GetPointer<port_o>(comp))
         GetPointer<port_o>(comp)->remove_connection(obj);
      if(GetPointer<signal_o>(comp))
         GetPointer<signal_o>(comp)->remove_port(obj);
   }
}

void structural_manager::remove_module(structural_objectRef obj)
{
   structural_objectRef top_obj = this->get_circ();
   CustomOrderedSet<structural_objectRef> remove;

   auto* obj_mod = GetPointer<module>(obj);
   // std::cerr << "obj: " << obj->get_path() << std::endl;
   // std::cerr << "removing inport" << std::endl;
   for(unsigned int i = 0; i < obj_mod->get_in_port_size(); i++)
   {
      structural_objectRef comp_port = obj_mod->get_in_port(i);
      if(comp_port->get_kind() == port_o_K)
      {
         remove_port_connection(comp_port);
      }
      else if(comp_port->get_kind() == port_vector_o_K)
      {
         for(unsigned int p = 0; p < GetPointer<port_o>(comp_port)->get_ports_size(); p++)
         {
            remove_port_connection(GetPointer<port_o>(comp_port)->get_port(p));
         }
      }
   }
   // std::cerr << "removing outport" << std::endl;
   for(unsigned int i = 0; i < obj_mod->get_out_port_size(); i++)
   {
      structural_objectRef comp_port = obj_mod->get_out_port(i);
      if(comp_port->get_kind() == port_o_K)
      {
         remove_port_connection(comp_port);
      }
      else if(comp_port->get_kind() == port_vector_o_K)
      {
         for(unsigned int p = 0; p < GetPointer<port_o>(comp_port)->get_ports_size(); p++)
         {
            remove_port_connection(GetPointer<port_o>(comp_port)->get_port(p));
         }
      }
   }

   // std::cerr << "removing signals" << std::endl;
   auto* top = GetPointer<module>(top_obj);
   top->remove_internal_object(obj);
   for(const auto& k : remove)
      top->remove_internal_object(k);
}

void structural_manager::remove_connection(structural_objectRef, structural_objectRef)
{
   THROW_ERROR("Not yet implemented");
}

void structural_manager::change_connection(structural_objectRef old_obj, structural_objectRef new_obj, structural_objectRef owner)
{
   auto* p_old = GetPointer<port_o>(old_obj);
   THROW_ASSERT(p_old, "Only port can change their connection");
   THROW_ASSERT(new_obj, "New connection has to be a valid one");
   // std::cerr << "change connection of: " << old_obj->get_path() << " with port " << new_obj->get_path() << std::endl;
   for(unsigned int i = 0; i < p_old->get_connections_size(); i++)
   {
      structural_objectRef conn_comp = p_old->get_connection(i);
      // std::cerr << "change connection between " << old_obj->get_path() << " and " << conn_comp->get_path() << " - will be conneted to " << new_obj->get_path() << std::endl;
      if(GetPointer<signal_o>(conn_comp) and GetPointer<signal_o>(conn_comp)->get_owner()->get_kind() != signal_vector_o_K and conn_comp->get_owner() != owner)
         continue;
      if(GetPointer<port_o>(conn_comp) and conn_comp->get_owner() != owner and conn_comp->get_owner()->get_owner() != owner and (conn_comp->get_owner()->get_kind() != port_vector_o_K or conn_comp->get_owner()->get_owner() != owner))
         continue;
      if(GetPointer<port_o>(new_obj) and !GetPointer<port_o>(new_obj)->is_connected(conn_comp))
         add_connection(new_obj, conn_comp);
      if(GetPointer<port_o>(conn_comp))
         GetPointer<port_o>(conn_comp)->substitute_connection(old_obj, new_obj);
      else if(GetPointer<signal_o>(conn_comp))
      {
         GetPointer<signal_o>(conn_comp)->substitute_port(old_obj, new_obj);
      }
      else
         THROW_ERROR("Connected component not supported: " + std::string(conn_comp->get_kind_text()));
   }
}
