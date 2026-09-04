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
 *   Copyright (C) 2012-2026 Politecnico di Milano
 *
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 *
 */
/**
 * @author Marco Minutoli <mminutoli@gmail.com>
 * @file
 * @brief Module for the generation of the top WB4 component.
 */

#ifndef WB4INTERCON_INTERFACE_H
#define WB4INTERCON_INTERFACE_H

#include "WB4_interface.hpp"

/**
 * @brief Class generating the top WB4 module.
 */
class WB4Intercon_interface : public WB4_interface
{
 public:
   /**
    * Constructor.
    */
   WB4Intercon_interface(const ParameterConstRef Param, const HLS_managerRef HLSManager, unsigned int functionId,
                         const DesignFlowManager& design_flow_manager);

   virtual void exec();
};

#endif /* WB4INTERCON_INTERFACE_H */
