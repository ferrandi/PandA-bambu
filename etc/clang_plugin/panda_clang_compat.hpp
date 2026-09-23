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
 *              Copyright (C) 2026 Politecnico di Milano
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
#ifndef PANDA_CLANG_COMPAT_HPP
#define PANDA_CLANG_COMPAT_HPP

#if defined(PANDA_CLANG_MAJOR)
#define PANDA_LLVM_CLANG_MAJOR PANDA_CLANG_MAJOR
#elif defined(__clang_major__)
#define PANDA_LLVM_CLANG_MAJOR __clang_major__
#else
#error "PANDA_LLVM_CLANG_MAJOR is undefined"
#endif

#if defined(PANDA_CLANG_VERSION_STR)
#define PANDA_LLVM_CLANG_VERSION_STR PANDA_CLANG_VERSION_STR
#elif defined(__clang_version__)
#define PANDA_LLVM_CLANG_VERSION_STR __clang_version__
#else
#define PANDA_LLVM_CLANG_VERSION_STR "unknown"
#endif

#endif // PANDA_CLANG_COMPAT_HPP
