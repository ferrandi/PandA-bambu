#include <catch.hpp>

#include <mockturtle/io/write_aiger.hpp>
#include <mockturtle/networks/aig.hpp>

template<
    typename T,
    typename Traits = std::char_traits<T>,
    typename Container = std::vector<T>>
struct seq_buffer : std::basic_streambuf<T, Traits>
{
  using base_type = std::basic_streambuf<T, Traits>;
  using int_type = typename base_type::int_type;
  using traits_type = typename base_type::traits_type;

  virtual int_type overflow( int_type ch )
  {
    if ( traits_type::eq_int_type( ch, traits_type::eof() ) )
    {
      return traits_type::eof();
    }
    c.push_back( traits_type::to_char_type( ch ) );
    return ch;
  }

  Container const& data() const
  {
    return c;
  }

private:
  Container c;
};

using namespace mockturtle;

TEST_CASE( "write single-gate AIG into AIGER file", "[write_aiger]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_or( a, b );
  aig.create_po( f1 );

  seq_buffer<char> buffer;
  std::ostream os( &buffer );
  write_aiger( aig, os );

  CHECK( buffer.data() ==
         std::vector<char>{
             0x61, 0x69, 0x67, 0x20, // aig
             0x33, 0x20,             // M=3 (I+L+A)
             0x32, 0x20,             // I=2
             0x30, 0x20,             // L=0
             0x31, 0x20,             // O=1
             0x31, 0x0a,             // A=1
             0x37, 0x0a,             // 1 PO
             0x01, 0x02,             // 1 AND gate
             0x63                    // comment
         } );
}

TEST_CASE( "write AIG for XOR into AIGERfile", "[write_aiger]" )
{
  aig_network aig;

  const auto a = aig.create_pi();
  const auto b = aig.create_pi();

  const auto f1 = aig.create_nand( a, b );
  const auto f2 = aig.create_nand( a, f1 );
  const auto f3 = aig.create_nand( b, f1 );
  const auto f4 = aig.create_nand( f2, f3 );
  aig.create_po( f4 );

  seq_buffer<char> buffer;
  std::ostream os( &buffer );
  write_aiger( aig, os );
  write_aiger( aig, "test.aig" );

  CHECK( buffer.data() ==
         std::vector<char>{
             0x61, 0x69, 0x67, 0x20, // aig
             0x36, 0x20,             // M=6 (I+L+A)
             0x32, 0x20,             // I=2
             0x30, 0x20,             // L=0
             0x31, 0x20,             // O=1
             0x34, 0x0a,             // A=4
             0x31, 0x33, 0x0a,       // 1 PO
             0x02, 0x02,             // 4 AND gates
             0x01, 0x05,
             0x03, 0x03,
             0x01, 0x02,
             0x63 // comment
         } );
}
