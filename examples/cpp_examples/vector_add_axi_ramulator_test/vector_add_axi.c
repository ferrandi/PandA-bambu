// Copyright (C) 2026 Jesun Firoz
// SPDX-License-Identifier: Apache-2.0
// Author: Jesun Firoz <jesun.firoz@pnnl.gov>

#pragma HLS interface port = vec_a mode = m_axi offset = direct
#pragma HLS interface port = vec_b mode = m_axi offset = direct
#pragma HLS interface port = vec_c mode = m_axi offset = direct

void vector_add(int* vec_a, int* vec_b, int* vec_c, int n)
{
   for(int i = 0; i < n; i++)
   {
      vec_c[i] = vec_a[i] + vec_b[i];
   }
}
