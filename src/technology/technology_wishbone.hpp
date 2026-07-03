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
 *              Copyright (C) 2012-2026 Politecnico di Milano
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
 * @file technology_wishbone.hpp
 * @brief Utility header to access wishbone technology library
 *
 * @author Marco Minutoli <mminutoli@gmail.com>
 *
 */
#ifndef TECHNOLOGY_WISHBONE_HPP
#define TECHNOLOGY_WISHBONE_HPP

#define WBLIBRARY "WBWrapper"

// Components
#define ADDRESS_RANGE_CHECKER_WB "address_range_checker_FU"
#define ADDRESS_FILTER_WB "address_filter_FU"
#define SEL_CONVERTER_WB "sel_converter_FU"
#define DATARAMSIZE_CONVERTER_WB "dataramsize_converter_FU"
#define REGISTER_WADDRESS_WB "register_waddress_FU"
#define WISHBONE_START_CONTROLLER_WB "wishbone_start_controller_FU"
#define NOT_INTERNAL_COMPONENT "not_internal_component"
#define NOTIFY_CALLER "notify_caller_FU"
#define WB4_INTERCON "wb4Intercon"

// Interface Signals
#define WB_DATIM_PORT_NAME "dat_im"
#define WB_ACKIM_PORT_NAME "ack_im"

#define WB_CYCOM_PORT_NAME "cyc_om"
#define WB_STBOM_PORT_NAME "stb_om"
#define WB_WEOM_PORT_NAME "we_om"
#define WB_ADDROM_PORT_NAME "addr_om"
#define WB_DATOM_PORT_NAME "dat_om"
#define WB_SELOM_PORT_NAME "sel_om"

#define WB_CYCIS_PORT_NAME "cyc_is"
#define WB_STBIS_PORT_NAME "stb_is"
#define WB_WEIS_PORT_NAME "we_is"
#define WB_ADDRIS_PORT_NAME "addr_is"
#define WB_DATIS_PORT_NAME "dat_is"
#define WB_SELIS_PORT_NAME "sel_is"

#define WB_DATOS_PORT_NAME "dat_os"
#define WB_ACKOS_PORT_NAME "ack_os"

#define WB_IRQ_PORT_NAME "irq"

// Parameters
#define WB_BASE_ADDRESS "BASE_ADDRESS"

#endif
