//    Copyright (C) 2023-2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//    This file is part of the PandA/Bambu MDPI Library.
//
//    author Michele Fiorito <michele.fiorito@polimi.it>
//
// Licensed under the Apache License, Version 2.0, with BAMBU exceptions (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
#ifndef __MDPI_MEMMAP_H
#define __MDPI_MEMMAP_H

#include "mdpi_types.h"

#include <cstddef>

class memmap
{
 protected:
   memmap() = default;

 public:
   virtual ~memmap() = default;

   virtual void check() = 0;

   virtual int map(ptr_t dst, void* src, size_t bytes) = 0;

   virtual bptr_t addrmap(ptr_t sim_addr) = 0;

   virtual ptr_t mapaddr(const bptr_t addr) = 0;
};

#endif // __MDPI_MEMMAP_H