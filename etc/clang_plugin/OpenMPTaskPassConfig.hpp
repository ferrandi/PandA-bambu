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
 * @file OpenMPTaskPassConfig.hpp
 *
 * @author Kaixi Matteo Chen <kaiximatteo.chen@mail.polimi.it>
 *
 */

#ifndef OPENMP_TASK_PASS_CONFIG_HPP
#define OPENMP_TASK_PASS_CONFIG_HPP

#define VERBOSE
#define SYSTEM_32_BIT
#define REMOVE_OUTLINED_GEP
#define SHOW_INPUT_PROXIES
// Currently this is handled inside bambu FU binding
#define DISABLE_RESOURCE_DUPLICATION

#endif