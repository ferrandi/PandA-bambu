// Copyright (C) 2026 Politecnico di Milano
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include <mdpi/mdpi.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(void)
{
   svLogicVecVal data[128] = {{0}};
   assert((m_next(1) & 255) == 2);
   if(getenv("MDPI_TEST_NULL_COMPLETION"))
   {
      m_read_complete(0, NULL);
   }
   else
   {
      m_read_complete_ch(getenv("MDPI_TEST_PLAIN") ? 0 : 123, 0, data);
   }
   fputs("Completion polling unexpectedly returned after an MDPI error\n", stderr);
   return EXIT_SUCCESS;
}
