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
 * @file XilinxWrapper.hpp
 * @brief Wrapper to synthesis tools by Xilinx
 *
 * A object used to invoke synthesis tools by Xilinx
 *
 * @author Christian Pilato <pilato@elet.polimi.it>
 * $Date$
 * Last modified by $Author$
 *
 */
#ifndef _XILINX_WRAPPER_HPP_
#define _XILINX_WRAPPER_HPP_

/// superclass include
#include "SynthesisTool.hpp"

#include "refcount.hpp"
REF_FORWARD_DECL(target_device);

#define UCF_SUBDIR (out_dir + std::string("/ucf/"))
#define XST_SUBDIR (out_dir + std::string("/xst/"))
#define NGDBUILD_SUBDIR (out_dir + std::string("/ngdbuild/"))
#define MAP_SUBDIR (out_dir + std::string("/map/"))
#define TRCE_SUBDIR (out_dir + std::string("/trce/"))
#define PAR_SUBDIR (out_dir + std::string("/par/"))
#define XPWR_SUBDIR (out_dir + std::string("/xpwr/"))
#define BITGEN_SUBDIR (out_dir + std::string("/bitgen/"))
#define PLANAHEAD_SUBDIR (out_dir + std::string("/planahead/"))
#define NETGEN_XST_SUBDIR (out_dir + std::string("/netgen_xst/"))
#define NETGEN_MAP_SUBDIR (out_dir + std::string("/netgen_map/"))
#define NETGEN_PAR_SUBDIR (out_dir + std::string("/netgen_par/"))

#define PARAM_xcf_file "xcf_file"
#define PARAM_xst_report "xst_report"
#define PARAM_xst_log_file "xst_log_file"

#define PARAM_ise_style "ise_style"

#define DEFAULT_INTSTYLE INTSTYLE_SILENT
#define INTSTYLE_ISE "ise"
#define INTSTYLE_SILENT "silent"
#define INTSTYLE_XFLOW "xflow"

class XilinxWrapper : public SynthesisTool
{
 public:
   /**
    * Constructor
    * @param Param is the set of parameters
    * @param tool_exec is the name of the executable
    * @param out_dir is the directory where to save all the results
    * @param default_out_dir is the default output directory
    */
   XilinxWrapper(const ParameterConstRef& Param, const std::string& tool_exec, const target_deviceRef& device, const std::string& out_dir, const std::string& default_out_dir);

   /**
    * Destructor
    */
   ~XilinxWrapper() override;

   /**
    * Creates the proper configuration script
    */
   void generate_synthesis_script(const DesignParametersRef& dp, const std::string& file_name) override;

   /**
    * Returns the string-based representation of the XML element
    */
   std::string toString(const xml_script_node_tRef node, const DesignParametersRef dp) const override;

   /**
    * Returns the string-based representation of the XML element
    */
   std::string getStringValue(const xml_script_node_tRef node, const DesignParametersRef& dp) const override;
};
/// Refcount definition for the class
typedef refcount<XilinxWrapper> XilinxWrapperRef;

#endif
