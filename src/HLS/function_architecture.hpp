/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
/**
 * @file function_architecture.hpp
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 */
#ifndef FUNCTION_ARCHITECTURE_HPP
#define FUNCTION_ARCHITECTURE_HPP

#include "utility/refcount.hpp"

#include <boost/preprocessor/seq/for_each.hpp>

#include <map>
#include <string>

#define ENUM_ID(r, data, elem) elem,
#define FUNC_ARCH_ATTR_ENUM                                                                             \
   (func_symbol)(func_name)(func_inline)(func_dataflow_top)(func_dataflow_module)(func_pipeline_style)( \
       func_pipeline_ii)(func_csroa)(func_original)
#define FUNC_ARCH_PARM_ATTR_ENUM                                                                                \
   (parm_port)(parm_index)(parm_bundle)(parm_offset)(parm_includes)(parm_typename)(parm_original_typename)(     \
       parm_elem_count)(parm_size_in_bytes)(parm_bank_allocation)(parm_array_dims)(parm_array_partition_types)( \
       parm_array_partition_factors)(parm_source_restrict)(parm_extent_known)
#define FUNC_ARCH_IFACE_ATTR_ENUM                                                                                  \
   (iface_name)(iface_global)(iface_mode)(iface_direction)(iface_bitwidth)(iface_alignment)(iface_depth)(          \
       iface_register)(iface_cache_ways)(iface_cache_line_count)(iface_cache_line_size)(                           \
       iface_cache_num_write_outstanding)(iface_cache_rep_policy)(iface_cache_bus_size)(iface_cache_write_policy)( \
       iface_cache_word_size)(iface_bank_number)(iface_chunk_size)(iface_max_read_burst_length)(                   \
       iface_max_write_burst_length)(iface_num_read_outstanding)(iface_read_fifo_depth)(                           \
       iface_write_burst_outstanding)(iface_write_fifo_depth)(iface_latency)

REF_FORWARD_DECL(FunctionArchitecture);

class FunctionArchitecture
{
 public:
   enum func_attr
   {
      BOOST_PP_SEQ_FOR_EACH(ENUM_ID, BOOST_PP_EMPTY, FUNC_ARCH_ATTR_ENUM)
   };
   static enum func_attr to_func_attr(const std::string& attr);

   enum parm_attr
   {
      BOOST_PP_SEQ_FOR_EACH(ENUM_ID, BOOST_PP_EMPTY, FUNC_ARCH_PARM_ATTR_ENUM)
   };
   static enum parm_attr to_parm_attr(const std::string& attr);

   enum iface_attr
   {
      BOOST_PP_SEQ_FOR_EACH(ENUM_ID, BOOST_PP_EMPTY, FUNC_ARCH_IFACE_ATTR_ENUM)
   };
   static enum iface_attr to_iface_attr(const std::string& attr);

   using func_attrs = std::map<enum func_attr, std::string>;
   using parm_attrs = std::map<enum parm_attr, std::string>;
   using iface_attrs = std::map<enum iface_attr, std::string>;

   func_attrs attrs;
   std::map<std::string, parm_attrs> parms;
   std::map<std::string, iface_attrs> ifaces;
};

#endif // FUNCTION_ARCHITECTURE_HPP
