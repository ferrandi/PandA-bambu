/*
 *
 *        _/_/_/    _/_/   _/    _/ _/_/_/    _/_/
 *       _/   _/ _/    _/ _/_/  _/ _/   _/ _/    _/
 *      _/_/_/  _/_/_/_/ _/  _/_/ _/   _/ _/_/_/_/
 *     _/      _/    _/ _/    _/ _/   _/ _/    _/
 *    _/      _/    _/ _/    _/ _/_/_/  _/    _/
 *
 *  ***********************************************
 *                   PandA Project
 *   URL: https://github.com/ferrandi/PandA-bambu
 *            Politecnico di Milano - DEIB
 *             System Architectures Group
 *  ***********************************************
 *   Copyright (C) 2025-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @file BackendWrapper.hpp
 * @brief This class handles the instantiation of the backend environment based on target device information
 *
 * @author Michele Fiorito <michele.fiorito@polimi.it>
 *
 */
#ifndef _BACKEND_FLOW_HPP_
#define _BACKEND_FLOW_HPP_

#include "area_info.hpp"
#include "custom_set.hpp"
#include "refcount.hpp"
#include "time_info.hpp"

#include <pugixml.hpp>

#include <filesystem>
#include <list>
#include <string>

CONSTREF_FORWARD_DECL(Parameter);
REF_FORWARD_DECL(generic_device);
REF_FORWARD_DECL(structural_manager);
REF_FORWARD_DECL(HLS_manager);

class BackendWrapper
{
   const ParameterConstRef Param;

   const generic_deviceRef device;

   const std::vector<std::string> backend_ids;

   const int debug_level;

   const int output_level;

   std::filesystem::path backend_launch_filename;

 public:
   /**
    * Constructor
    * @param Param application parameters object
    * @param target device the backend flow will target
    * @param backend_ids backend flows to use among those available
    */
   BackendWrapper(const ParameterConstRef& Param, const generic_deviceRef& target,
                  const std::vector<std::string>& backend_ids);

   void init(const HLS_managerRef& HLSMgr);

   void init(const std::string& fu_name, const structural_managerRef SM, const std::list<std::string>& hdl_files);

   void init(const std::string& top_fu_name, const std::string& top_hls_name, const std::list<std::string>& hdl_files,
             const HLS_managerRef& HLSMgr = HLS_managerRef());

   pugi::xml_document run() const;

   std::string get_flow_name() const;

   static pugi::xml_document LoadResults(const ParameterConstRef& Param);

   static void StoreResults(const pugi::xml_document& res, const ParameterConstRef& Param);

   static void ParseResults(const pugi::xml_document& res, area_info& resource_info, time_info& timing_info);
};

#endif
