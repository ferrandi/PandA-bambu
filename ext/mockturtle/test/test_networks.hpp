#pragma once

#include <cstdint>

namespace mockturtle
{

class dummy_network
{
public:
  using base_type = dummy_network;
  using node = uint32_t;
  using signal = uint32_t;
  using storage = uint64_t;

  static constexpr uint32_t min_fanin_size = 1;
  static constexpr uint32_t max_fanin_size = 10;

  dummy_network( uint64_t& storage )
    : _storage{storage}
  {
  }

public:
  uint64_t& _storage;
};

}
