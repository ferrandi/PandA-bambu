//    Copyright (C) 2025-2026 Politecnico di Milano
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
#ifndef __MDPI_STACK_H
#define __MDPI_STACK_H
#include <stdio.h>
#include <stdlib.h>

typedef struct
{
   void** data;
   size_t size;
   size_t capacity;
} m_stack_t;

static int m_stack_init(m_stack_t* s, size_t initial_capacity)
{
   s->data = (void**)malloc(initial_capacity * sizeof(void*));
   if(!s->data)
   {
      return -1;
   }
   s->size = 0;
   s->capacity = initial_capacity;
   return 0;
}

static int m_stack_init_copy(m_stack_t* s, const m_stack_t* other)
{
   s->data = (void**)malloc(other->capacity * sizeof(void*));
   if(!s->data)
   {
      return -1;
   }
   s->size = other->size;
   s->capacity = other->capacity;
   memcpy(s->data, other->data, (other->size - 1) * sizeof(void*));
   return 0;
}

static int m_stack_push(m_stack_t* s, void* value)
{
   if(s->size == s->capacity)
   {
      s->capacity *= 2;
      s->data = (void**)realloc(s->data, s->capacity * sizeof(void*));
      if(!s->data)
      {
         return -1;
      }
   }
   s->data[s->size++] = value;
   return 0;
}

static int m_stack_pop(m_stack_t* s, void** out)
{
   if(s->size == 0)
   {
      return -1; // stack underflow
   }
   *out = s->data[--s->size];
   return 0;
}

static int m_stack_front(m_stack_t* s, void** out)
{
   if(s->size)
   {
      *out = s->data[s->size - 1];
      return 0;
   }
   return -1;
}

static int m_stack_is_empty(const m_stack_t* s)
{
   return s->size == 0;
}

static void m_stack_fini(m_stack_t* s)
{
   free(s->data);
}
#endif // __MDPI_STACK_H
