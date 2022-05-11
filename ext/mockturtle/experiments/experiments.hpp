/* mockturtle: C++ logic network library
 * Copyright (C) 2018-2021  EPFL
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */

/*!
  \file experiments.hpp
  \brief Framework for simple experimental evaluation

  \author Mathias Soeken
*/

#include <array>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

#include <fmt/color.h>
#include <fmt/format.h>
#include <mockturtle/io/write_bench.hpp>
#include <nlohmann/json.hpp>

namespace experiments
{

struct json_table
{
  explicit json_table( nlohmann::json const& data, std::vector<std::string> const& columns )
      : columns_( columns )
  {
    for ( auto const& column : columns )
    {
      max_widths_.push_back( static_cast<uint32_t>( column.size() ) );
    }
    entries_.push_back( columns );
    for ( auto const& row : data )
    {
      add_row( row );
    }
  }

  void print( std::ostream& os )
  {
    for ( const auto& entry : entries_ )
    {
      os << "|";
      for ( auto i = 0u; i < entry.size(); ++i )
      {
        os << fmt::format( " {1:>{0}} |", max_widths_[i], entry[i] );
      }
      os << "\n";
    }
  }

private:
  void add_row( nlohmann::json const& row )
  {
    std::vector<std::string> entry;
    uint32_t ctr{0u};
    for ( auto const& key : columns_ )
    {
      auto const& data = row[key];
      std::string cell;

      if ( data.is_string() )
      {
        cell = static_cast<std::string>( data );
      }
      else if ( data.is_number_integer() )
      {
        cell = std::to_string( static_cast<int>( data ) );
      }
      else if ( data.is_number() )
      {
        cell = fmt::format( "{:.2f}", static_cast<float>( data ) );
      }
      else if ( data.is_boolean() )
      {
        cell = fmt::format( "{}", static_cast<bool>( data ) );
      }

      max_widths_[ctr] = std::max( max_widths_[ctr], static_cast<uint32_t>( cell.size() ) );
      ++ctr;
      entry.push_back( cell );
    }
    entries_.push_back( entry );
  }

private:
  std::vector<uint32_t> max_widths_;
  std::vector<std::string> columns_;
  std::vector<std::vector<std::string>> entries_;
};

static constexpr const char* use_github_revision = "##GITHUB##";

template<typename T, typename... Ts>
struct first_type
{
  using type = T;
};

template<typename... Ts>
using first_type_t = typename first_type<Ts...>::type;

template<typename... ColumnTypes>
class experiment
{
public:
  template<typename... T>
  explicit experiment( std::string_view name, T... column_names )
      : name_( name )
  {
    static_assert( ( sizeof...( ColumnTypes ) > 0 ), "at least one column must be specified" );
    static_assert( ( sizeof...( ColumnTypes ) == sizeof...( T ) ), "number of column names must match column types" );
    static_assert( ( std::is_constructible_v<std::string, T> && ... ), "all column names must be strings" );
    ( column_names_.push_back( column_names ), ... );

#ifndef EXPERIMENTS_PATH
    filename_ = fmt::format( "{}.json", name );
#else
    filename_ = fmt::format( "{}{}.json", EXPERIMENTS_PATH, name );
#endif

    std::ifstream in( filename_, std::ifstream::in );
    if ( in.good() )
    {
      data_ = nlohmann::json::parse( in );
    }
  }

  void save( std::string_view version = use_github_revision )
  {
    nlohmann::json entries;
    for ( auto const& row : rows_ )
    {
      auto it = column_names_.begin();
      nlohmann::json entry;
      std::apply(
          [&]( auto&&... args ) {
            ( ( entry[*it++] = args ), ... );
          },
          row );
      entries.push_back( entry );
    }

    std::string version_;
    version_ = version;
#ifdef GIT_SHORT_REVISION
    if ( version == experiments::use_github_revision )
    {
      version_ = GIT_SHORT_REVISION;
    }
#endif

    if ( !data_.empty() && data_.back()["version"] == version_ )
    {
      data_.erase( data_.size() - 1u );
    }

    data_.push_back( {{"version", version_},
                      {"entries", entries}} );

    std::ofstream os( filename_, std::ofstream::out );
    os << data_.dump( 2 ) << "\n";
  }

  void operator()( ColumnTypes... args )
  {
    rows_.emplace_back( args... );
  }

  nlohmann::json const& dataset( std::string const& version, nlohmann::json const& def ) const
  {
    if ( version.empty() )
    {
      return def;
    }
    else
    {
      if ( const auto it = std::find_if( data_.begin(), data_.end(), [&]( auto const& entry ) { return entry["version"] == version; } ); it != data_.end() )
      {
        return *it;
      }
      else
      {
        throw std::exception();
      }
    }
  }

  bool table( std::string const& version = {}, std::ostream& os = std::cout ) const
  {
    if ( data_.empty() )
    {
      fmt::print( "[w] no data available\n" );
      return false;
    }

    try
    {
      auto const& data = dataset( version, data_.back() );
      fmt::print( "[i] dataset " );
      fmt::print( fg( fmt::terminal_color::blue ), "{}\n", data["version"] );

      json_table( data["entries"], column_names_ ).print( os );
    }
    catch ( ... )
    {
      fmt::print( "[w] version {} not found\n", version );
      return false;
    }

    return true;
  }

  bool compare( std::string const& old_version = {},
                std::string const& current_version = {},
                std::vector<std::string> const& track_columns = {},
                std::ostream& os = std::cout )
  {
    if ( data_.size() < 2u )
    {
      fmt::print( "[w] dataset contains less than two entry sets\n" );
      return false;
    }

    try
    {
      auto const& data_old = dataset( old_version, data_[data_.size() - 2u] );
      auto const& data_cur = dataset( current_version, data_.back() );

      auto const& entries_old = data_old["entries"];
      auto const& entries_cur = data_cur["entries"];

      fmt::print( "[i] compare " );
      fmt::print( fg( fmt::terminal_color::blue ), "{}", data_old["version"] );
      fmt::print( " to " );
      fmt::print( fg( fmt::terminal_color::blue ), "{}\n", data_cur["version"] );

      /* collect keys */
      using first_t = first_type_t<ColumnTypes...>;
      std::vector<first_t> keys;
      for ( auto const& entry : entries_cur )
      {
        nlohmann::json const& j = entry[column_names_.front()];
        keys.push_back( j.get<first_t>() );
      }
      for ( auto const& entry : entries_old )
      {
        nlohmann::json const& j = entry[column_names_.front()];
        auto value = j.get<first_t>();
        if ( std::find( keys.begin(), keys.end(), value ) == keys.end() )
        {
          keys.push_back( value );
        }
      }

      /* track differences */
      std::unordered_map<std::string, uint32_t> differences;
      for ( auto const& column : track_columns )
      {
        differences[column] = 0u;
      }

      /* prepare entries */
      auto find_key = [&]( nlohmann::json const& entries, first_t const& key ) {
        return std::find_if( entries.begin(), entries.end(), [&]( auto const& entry ) {
          nlohmann::json const& j = entry[column_names_.front()];
          return j.get<first_t>() == key;
        } );
      };

      auto compare_columns = column_names_;
      std::transform( column_names_.begin() + 1, column_names_.end(), std::back_inserter( compare_columns ),
                      []( auto const& name ) { return name + "'"; } );

      nlohmann::json compare_entries;
      for ( auto const& key : keys )
      {
        nlohmann::json row;
        const auto it_old = find_key( entries_old, key );
        if ( it_old != entries_old.end() )
        {
          row = *it_old;
        }
        if ( auto const it = find_key( entries_cur, key ); it != entries_cur.end() )
        {
          if ( it_old == entries_old.end() )
          {
            row[column_names_[0]] = (*it)[column_names_[0]];
          }
          for ( auto i = 1u; i < column_names_.size(); ++i )
          {
            row[column_names_[i] + "'"] = (*it)[column_names_[i]];

            if ( it_old != entries_old.end() )
            {
              if ( const auto it_diff = differences.find( column_names_[i] ); it_diff != differences.end() && row[column_names_[i]] != row[column_names_[i] + "'"] )
              {
                it_diff->second++;
              }
            }
          }
        }
        compare_entries.push_back( row );
      }

      json_table( compare_entries, compare_columns ).print( os );

      for ( const auto& [k, v] : differences ) {
        if ( v == 0u ) {
          os << fmt::format( "[i] no differences in column '{}'\n", k );
        } else {
          os << fmt::format( "[i] {} differences in column '{}'\n", v, k );
        }
      }
    }
    catch ( ... )
    {
      fmt::print( "[w] dataset not found\n" );
      return false;
    }

    return true;
  }

private:
  std::string name_;
  std::string filename_;
  std::vector<std::string> column_names_;
  std::vector<std::tuple<ColumnTypes...>> rows_;

  nlohmann::json data_;
};

// clang-format off
/* EPFL benchmarks */
static constexpr uint64_t adder           = 0b0000000000000000000000000000000000000000000000000000000000000001;
static constexpr uint64_t bar             = 0b0000000000000000000000000000000000000000000000000000000000000010;
static constexpr uint64_t div             = 0b0000000000000000000000000000000000000000000000000000000000000100;
static constexpr uint64_t hyp             = 0b0000000000000000000000000000000000000000000000000000000000001000;
static constexpr uint64_t log2            = 0b0000000000000000000000000000000000000000000000000000000000010000;
static constexpr uint64_t max             = 0b0000000000000000000000000000000000000000000000000000000000100000;
static constexpr uint64_t multiplier      = 0b0000000000000000000000000000000000000000000000000000000001000000;
static constexpr uint64_t sin             = 0b0000000000000000000000000000000000000000000000000000000010000000;
static constexpr uint64_t sqrt            = 0b0000000000000000000000000000000000000000000000000000000100000000;
static constexpr uint64_t square          = 0b0000000000000000000000000000000000000000000000000000001000000000;
static constexpr uint64_t arbiter         = 0b0000000000000000000000000000000000000000000000000000010000000000;
static constexpr uint64_t cavlc           = 0b0000000000000000000000000000000000000000000000000000100000000000;
static constexpr uint64_t ctrl            = 0b0000000000000000000000000000000000000000000000000001000000000000;
static constexpr uint64_t dec             = 0b0000000000000000000000000000000000000000000000000010000000000000;
static constexpr uint64_t i2c             = 0b0000000000000000000000000000000000000000000000000100000000000000;
static constexpr uint64_t int2float       = 0b0000000000000000000000000000000000000000000000001000000000000000;
static constexpr uint64_t mem_ctrl        = 0b0000000000000000000000000000000000000000000000010000000000000000;
static constexpr uint64_t priority        = 0b0000000000000000000000000000000000000000000000100000000000000000;
static constexpr uint64_t router          = 0b0000000000000000000000000000000000000000000001000000000000000000;
static constexpr uint64_t voter           = 0b0000000000000000000000000000000000000000000010000000000000000000;
static constexpr uint64_t arithmetic      = 0b0000000000000000000000000000000000000000000000000000001111111111;
static constexpr uint64_t random          = 0b0000000000000000000000000000000000000000000011111111110000000000;
static constexpr uint64_t epfl            = 0b0000000000000000000000000000000000000000000011111111111111111111;

/* IWLS 2005 benchmarks */
static constexpr uint64_t ac97_ctrl       = 0b0000000000000000000000000000000000000000000100000000000000000000;
static constexpr uint64_t aes_core        = 0b0000000000000000000000000000000000000000001000000000000000000000;
static constexpr uint64_t des_area        = 0b0000000000000000000000000000000000000000010000000000000000000000;
static constexpr uint64_t des_perf        = 0b0000000000000000000000000000000000000000100000000000000000000000;
static constexpr uint64_t DMA             = 0b0000000000000000000000000000000000000001000000000000000000000000;
static constexpr uint64_t DSP             = 0b0000000000000000000000000000000000000010000000000000000000000000;
static constexpr uint64_t ethernet        = 0b0000000000000000000000000000000000000100000000000000000000000000;
static constexpr uint64_t iwls05_i2c      = 0b0000000000000000000000000000000000001000000000000000000000000000;
static constexpr uint64_t leon2           = 0b0000000000000000000000000000000000010000000000000000000000000000;
static constexpr uint64_t leon3_opt       = 0b0000000000000000000000000000000000100000000000000000000000000000;
static constexpr uint64_t leon3           = 0b0000000000000000000000000000000001000000000000000000000000000000;
static constexpr uint64_t leon3mp         = 0b0000000000000000000000000000000010000000000000000000000000000000;
static constexpr uint64_t iwls05_mem_ctrl = 0b0000000000000000000000000000000100000000000000000000000000000000;
static constexpr uint64_t netcard         = 0b0000000000000000000000000000001000000000000000000000000000000000;
static constexpr uint64_t pci_bridge32    = 0b0000000000000000000000000000010000000000000000000000000000000000;
static constexpr uint64_t RISC            = 0b0000000000000000000000000000100000000000000000000000000000000000;
static constexpr uint64_t sasc            = 0b0000000000000000000000000001000000000000000000000000000000000000;
static constexpr uint64_t simple_spi      = 0b0000000000000000000000000010000000000000000000000000000000000000;
static constexpr uint64_t spi             = 0b0000000000000000000000000100000000000000000000000000000000000000;
static constexpr uint64_t ss_pcm          = 0b0000000000000000000000001000000000000000000000000000000000000000;
static constexpr uint64_t systemcaes      = 0b0000000000000000000000010000000000000000000000000000000000000000;
static constexpr uint64_t systemcdes      = 0b0000000000000000000000100000000000000000000000000000000000000000;
static constexpr uint64_t tv80            = 0b0000000000000000000001000000000000000000000000000000000000000000;
static constexpr uint64_t usb_funct       = 0b0000000000000000000010000000000000000000000000000000000000000000;
static constexpr uint64_t usb_phy         = 0b0000000000000000000100000000000000000000000000000000000000000000;
static constexpr uint64_t vga_lcd         = 0b0000000000000000001000000000000000000000000000000000000000000000;
static constexpr uint64_t wb_conmax       = 0b0000000000000000010000000000000000000000000000000000000000000000;
static constexpr uint64_t iwls            = 0b0000000000000000011111111111111111111111111100000000000000000000;

/* ISCAS benchmarks */
static constexpr uint64_t c17             = 0b0000000000000000100000000000000000000000000000000000000000000000;
static constexpr uint64_t c432            = 0b0000000000000001000000000000000000000000000000000000000000000000;
static constexpr uint64_t c499            = 0b0000000000000010000000000000000000000000000000000000000000000000;
static constexpr uint64_t c880            = 0b0000000000000100000000000000000000000000000000000000000000000000;
static constexpr uint64_t c1355           = 0b0000000000001000000000000000000000000000000000000000000000000000;
static constexpr uint64_t c1908           = 0b0000000000010000000000000000000000000000000000000000000000000000;
static constexpr uint64_t c2670           = 0b0000000000100000000000000000000000000000000000000000000000000000;
static constexpr uint64_t c3540           = 0b0000000001000000000000000000000000000000000000000000000000000000;
static constexpr uint64_t c5315           = 0b0000000010000000000000000000000000000000000000000000000000000000;
static constexpr uint64_t c6288           = 0b0000000100000000000000000000000000000000000000000000000000000000;
static constexpr uint64_t c7552           = 0b0000001000000000000000000000000000000000000000000000000000000000;
static constexpr uint64_t iscas           = 0b0000001111111111100000000000000000000000000000000000000000000000;

static constexpr uint64_t all             = 0b0000001111111111111111111111111111111111111111111111111111111111;
// clang-format on

static const char* benchmarks[] = {
    "adder", "bar", "div", "hyp", "log2", "max", "multiplier", "sin", "sqrt", "square",
    "arbiter", "cavlc", "ctrl", "dec", "i2c", "int2float", "mem_ctrl", "priority", "router", "voter",

    "ac97_ctrl", "aes_core", "des_area", "des_perf", "DMA", "DSP", "ethernet", "iwls05_i2c", "leon2",
    "leon3_opt", "leon3", "leon3mp", "iwls05_mem_ctrl", "netcard", "pci_bridge32", "RISC", "sasc",
    "simple_spi", "spi", "ss_pcm", "systemcaes", "systemcdes", "tv80", "usb_funct", "usb_phy",
    "vga_lcd", "wb_conmax",

    "c17", "c432", "c499", "c880", "c1355", "c1908", "c2670", "c3540", "c5315", "c6288", "c7552"};

std::vector<std::string> epfl_benchmarks( uint64_t selection = epfl )
{
  std::vector<std::string> result;
  for ( uint32_t i = 0u; i < 20u; ++i )
  {
    if ( ( selection >> i ) & 1 )
    {
      result.push_back( benchmarks[i] );
    }
  }
  return result;
}

std::vector<std::string> iwls_benchmarks( uint64_t selection = iwls )
{
  std::vector<std::string> result;
  for ( uint32_t i = 20u; i < 47u; ++i )
  {
    if ( ( selection >> i ) & 1 )
    {
      result.push_back( benchmarks[i] );
    }
  }
  return result;
}

std::vector<std::string> iscas_benchmarks( uint64_t selection = iscas )
{
  std::vector<std::string> result;
  for ( uint32_t i = 47u; i < 58u; ++i )
  {
    if ( ( selection >> i ) & 1 )
    {
      result.push_back( benchmarks[i] );
    }
  }
  return result;
}

std::vector<std::string> all_benchmarks( uint64_t selection = all )
{
  std::vector<std::string> result;
  for ( uint32_t i = 0u; i < 58u; ++i )
  {
    if ( ( selection >> i ) & 1 )
    {
      result.push_back( benchmarks[i] );
    }
  }
  return result;
}

std::string benchmark_path( std::string const& benchmark_name )
{
#ifndef EXPERIMENTS_PATH
  return fmt::format( "{}.aig", benchmark_name );
#else
  return fmt::format( "{}benchmarks/{}.aig", EXPERIMENTS_PATH, benchmark_name );
#endif
}

template<class Ntk>
inline bool abc_cec_impl( Ntk const& ntk, std::string const& benchmark_fullpath )
{
  mockturtle::write_bench( ntk, "/tmp/test.bench" );
  std::string command = fmt::format( "abc -q \"cec -n {} /tmp/test.bench\"", benchmark_fullpath );

  std::array<char, 128> buffer;
  std::string result;
  std::unique_ptr<FILE, decltype( &pclose )> pipe( popen( command.c_str(), "r" ), pclose );
  if ( !pipe )
  {
    throw std::runtime_error( "popen() failed" );
  }
  while ( fgets( buffer.data(), buffer.size(), pipe.get() ) != nullptr )
  {
    result += buffer.data();
  }

  /* search for one line which says "Networks are equivalent" and ignore all other debug output from ABC */
  std::stringstream ss( result );
  std::string line;
  while ( std::getline( ss, line, '\n' ) )
  {
    if ( line.size() >= 23u && line.substr( 0u, 23u ) == "Networks are equivalent" )
    {
      return true;
    }
  }

  return false;
}

template<class Ntk>
inline bool abc_cec( Ntk const& ntk, std::string const& benchmark )
{
  return abc_cec_impl( ntk, benchmark_path( benchmark ) );
}

} // namespace experiments
