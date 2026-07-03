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
 *              Copyright (C) 2025-2026 Politecnico di Milano
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
 * @file area_estimation.hpp
 * @brief Helpers to compute LUT-equivalent area, including DSP contributions.
 *
 * @author Fabrizio Ferrandi <fabrizio.ferrandi@polimi.it>
 *
 */
#ifndef AREA_ESTIMATION_HPP
#define AREA_ESTIMATION_HPP

#include "area_info.hpp"
#include "hls_device.hpp"

namespace area_estimation
{
   inline double get_dsp_lut_scale(const HLS_deviceConstRef& HLS_D)
   {
      constexpr double default_dsp_lut_scale = 350.0;
      if(!HLS_D)
      {
         return default_dsp_lut_scale;
      }
      return HLS_D->has_parameter("DSP_LUT_SCALE") ? HLS_D->get_parameter<double>("DSP_LUT_SCALE") :
                                                     default_dsp_lut_scale;
   }

   inline double get_lut_equivalent_area_weighted(const HLS_deviceConstRef& HLS_D, double lut_area, double dsp_count,
                                                  double dsp_weight_k = 1.0)
   {
      return lut_area + dsp_count * get_dsp_lut_scale(HLS_D) * dsp_weight_k;
   }

   inline double get_lut_component(const area_infoRef& area_m)
   {
      if(!area_m)
      {
         return 0.0;
      }

      double area = area_m->resource_or_default(area_info::SLICE_LUTS);
      if(area == 0.0)
      {
         area = area_m->resource_or_default(area_info::LUT);
         if(area == 0.0)
         {
            area = area_m->resource_or_default(area_info::AREA);
         }
      }
      return area;
   }

   inline double get_lut_equivalent_area_weighted(const HLS_deviceConstRef& HLS_D, const area_infoRef& area_m,
                                                  double dsp_weight_k = 1.0)
   {
      if(!area_m)
      {
         return 0.0;
      }
      return get_lut_equivalent_area_weighted(HLS_D, get_lut_component(area_m),
                                              area_m->resource_or_default(area_info::DSP), dsp_weight_k);
   }

   inline double get_lut_equivalent_area(const HLS_deviceConstRef& HLS_D, const area_infoRef& area_m)
   {
      return get_lut_equivalent_area_weighted(HLS_D, area_m, 1.0);
   }
} // namespace area_estimation

#endif
