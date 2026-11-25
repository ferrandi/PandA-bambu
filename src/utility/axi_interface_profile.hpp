/*
 * Part of the PandA Project, under the Apache License v2.0 with LLVM Exceptions.
 * SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
 */
#ifndef PANDA_AXI_INTERFACE_PROFILE_HPP
#define PANDA_AXI_INTERFACE_PROFILE_HPP

#include "bounded_decimal.hpp"

#include <algorithm>
#include <cstdint>
#include <string>

namespace panda
{
   namespace utility
   {
      constexpr std::uint32_t axi_burst_length_min = 1U;
      constexpr std::uint32_t axi_burst_length_max = 256U;
      constexpr std::uint32_t axi_burst_length_default = 16U;
      constexpr std::uint32_t axi_outstanding_min = 1U;
      constexpr std::uint32_t axi_outstanding_max = 16U;
      constexpr std::uint32_t axi_outstanding_default = 16U;
      constexpr std::uint32_t axi_fifo_depth_min = 1U;
      constexpr std::uint32_t axi_fifo_depth_max = 4096U;
      constexpr std::uint32_t axi_read_fifo_depth_default = 256U;
      constexpr std::uint32_t axi_write_fifo_depth_default = 32U;
      constexpr std::uint32_t axi_latency_max = UINT32_MAX;

      struct axi_read_profile
      {
         std::uint32_t max_burst_length = axi_burst_length_default;
         std::uint32_t max_outstanding = axi_outstanding_default;
         std::uint32_t fifo_depth = axi_read_fifo_depth_default;
      };

      struct axi_write_profile
      {
         std::uint32_t max_burst_length = axi_burst_length_default;
         std::uint32_t max_outstanding = axi_outstanding_default;
         std::uint32_t fifo_depth = axi_write_fifo_depth_default;
      };

      inline bool is_axi_fifo_depth_valid(const std::uint32_t depth)
      {
         return depth >= axi_fifo_depth_min && depth <= axi_fifo_depth_max && (depth & (depth - 1U)) == 0U;
      }

      inline std::uint32_t derive_axi_read_fifo_depth(const std::uint32_t burst_length, const std::uint32_t outstanding)
      {
         const auto count = std::max(2U, outstanding);
         const auto needed = count * burst_length;
         std::uint32_t depth = 1U;
         while(depth < needed && depth < axi_fifo_depth_max)
         {
            depth <<= 1U;
         }
         return depth;
      }

      inline std::uint32_t derive_axi_write_fifo_depth(const std::uint32_t burst_length)
      {
         const auto needed = 2U * burst_length;
         std::uint32_t depth = 1U;
         while(depth < needed)
         {
            depth <<= 1U;
         }
         return depth;
      }

      inline bounded_decimal_result parse_axi_profile_decimal(const std::string& text, const std::uint32_t minimum,
                                                              const std::uint32_t maximum, std::uint32_t& result)
      {
         return parse_bounded_ascii_decimal(text, minimum, maximum, result);
      }
   } // namespace utility
} // namespace panda

#endif // PANDA_AXI_INTERFACE_PROFILE_HPP
