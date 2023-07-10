#include <catch.hpp>

#include <vector>

#include <mockturtle/utils/cuts.hpp>

using namespace mockturtle;

TEST_CASE( "create cuts", "[cuts]" )
{
  std::vector<uint32_t> v{ 1, 2, 4, 5 };

  cut<51> c;
  c.set_leaves( v.begin(), v.begin() + 2 );

  CHECK( c.size() == 2 );
  CHECK( std::equal( v.begin(), v.begin() + 2, c.begin(), c.end() ) );
  CHECK( c.signature() == 6 );

  cut<51> c2;
  c2.set_leaves( std::vector<uint32_t>{ 1, 2, 3, 4, 5 } );

  CHECK( c2.size() == 5 );
}

TEST_CASE( "access data", "[cuts]" )
{
  std::vector<uint32_t> v{ 1, 2, 4, 5 };

  cut<51, uint32_t> c1;
  c1.set_leaves( v.begin(), v.end() );
  c1.data() = 42;

  CHECK( c1.size() == 4 );
  CHECK( c1.data() == 42 );

  struct S
  {
    uint8_t a;
    uint8_t b;
  };

  cut<51, S> c2;
  c2.set_leaves( v.begin(), v.end() );
  c2->a = 12;
  c2->b = 13;

  CHECK( c2.size() == 4 );
  CHECK( c2->a + c2->b == 25 );
}

TEST_CASE( "dominate cuts", "[cuts]" )
{
  std::vector<uint32_t> v{ 1, 2, 4, 5 };
  cut<51> c1, c2, c3;

  c1.set_leaves( v.begin(), v.begin() + 2 );
  c2.set_leaves( v.begin() + 2, v.begin() + 4 );
  c3.set_leaves( v.begin(), v.begin() + 4 );

  CHECK( c1.dominates( c1 ) );
  CHECK( !c1.dominates( c2 ) );
  CHECK( c1.dominates( c3 ) );
  CHECK( !c2.dominates( c1 ) );
  CHECK( c2.dominates( c2 ) );
  CHECK( c2.dominates( c3 ) );
  CHECK( !c3.dominates( c1 ) );
  CHECK( !c3.dominates( c2 ) );
  CHECK( c3.dominates( c3 ) );
}

TEST_CASE( "merge cuts", "[cuts]" )
{
  std::vector<uint32_t> v{ 1, 2, 3, 4 };

  cut<51> c1, c2, c3, r12, r13, r12t, r13f;

  c1.set_leaves( v.begin(), v.begin() + 2 );
  c2.set_leaves( v.begin() + 1, v.begin() + 3 );
  c3.set_leaves( v.begin() + 2, v.begin() + 4 );

  CHECK( c1.merge( c2, r12, 4 ) );
  CHECK( c1.merge( c3, r13, 4 ) );
  CHECK( c1.merge( c2, r12t, 3 ) );
  CHECK( !c1.merge( c3, r13f, 3 ) );

  CHECK( r12.size() == 3 );
  CHECK( r13.size() == 4 );
  CHECK( r12t.size() == 3 );
}

TEST_CASE( "create cut set", "[cuts]" )
{
  using cut_type = cut<10, uint32_t>;

  std::vector<uint32_t> v{ 1, 2, 3, 4, 5 };
  cut_set<cut_type, 25> set;
  set.add_cut( v.begin(), v.begin() + 2 ).data() = 23;
  set.add_cut( v.begin() + 2, v.end() ).data() = 37;

  std::vector<uint32_t> v2;
  uint32_t sum{};
  for ( auto const& c : set )
  {
    std::copy( c->begin(), c->end(), std::back_inserter( v2 ) );
    sum += c->data();
  }

  CHECK( v == v2 );
  CHECK( sum == 60 );
}

TEST_CASE( "insert a cut", "[cuts]" )
{
  using cut_type = cut<10>;

  cut_type c1, c2, c3, c4;
  c1.set_leaves( std::vector<uint32_t>{ 1, 2, 3 } );
  c2.set_leaves( std::vector<uint32_t>{ 1, 2, 3, 4 } );
  c3.set_leaves( std::vector<uint32_t>{ 1, 2 } );
  c4.set_leaves( std::vector<uint32_t>{ 3 } );

  cut_set<cut_type, 25> set;

  CHECK( set.size() == 0u );
  CHECK( !set.is_dominated( c1 ) );

  set.insert( c1 );
  CHECK( set.size() == 1u );
  CHECK( set.is_dominated( c2 ) );
  CHECK( !set.is_dominated( c3 ) );

  set.insert( c3 );
  CHECK( set.size() == 1u );

  auto it = set.begin();
  CHECK( ( *it )->size() == 2 );
  CHECK( std::vector<uint32_t>( ( *it )->begin(), ( *it )->end() ) == std::vector<uint32_t>{ 1, 2 } );

  CHECK( !set.is_dominated( c4 ) );
  set.insert( c4 );
  CHECK( set.size() == 2u );
  it = set.begin();
  CHECK( std::vector<uint32_t>( ( *it )->begin(), ( *it )->end() ) == std::vector<uint32_t>{ 3 } );
  ++it;
  CHECK( std::vector<uint32_t>( ( *it )->begin(), ( *it )->end() ) == std::vector<uint32_t>{ 1, 2 } );
}

TEST_CASE( "insert a cut2", "[cuts]" )
{
  using cut_type = cut<10>;

  cut_type c1, c2, c3, c4, c5;
  c1.set_leaves( std::vector<uint32_t>{ 3, 6 } );
  c2.set_leaves( std::vector<uint32_t>{ 1, 2, 3 } );
  c3.set_leaves( std::vector<uint32_t>{ 1, 2, 3, 8 } );
  c4.set_leaves( std::vector<uint32_t>{ 3, 4, 5 } );
  c5.set_leaves( std::vector<uint32_t>{ 7, 8 } );

  cut_set<cut_type, 25> set;

  CHECK( !set.is_dominated( c1 ) );
  set.insert( c1 );
  CHECK( set.size() == 1 );

  CHECK( !set.is_dominated( c2 ) );
  set.insert( c2 );
  CHECK( set.size() == 2 );

  CHECK( set.is_dominated( c3 ) );
  CHECK( set.size() == 2 );
}

TEST_CASE( "merge three cuts", "[cuts]" )
{
  using cut_type = cut<10>;

  cut_type c1, c2, c3, cr, ct;
  c1.set_leaves( std::vector{ 2u, 4u, 6u } );
  c2.set_leaves( std::vector{ 3u, 5u, 7u } );
  c3.set_leaves( std::vector{ 1u, 2u, 9u } );

  c1.merge( c2, cr, 10 );
  CHECK( std::vector<uint32_t>( cr.begin(), cr.end() ) == std::vector{ 2u, 3u, 4u, 5u, 6u, 7u } );

  // ct.set_leaves( std::vector<uint32_t>( cr.begin(), cr.end() ) );
  ct = cr;
  ct.merge( c3, cr, 10 );
  CHECK( std::vector<uint32_t>( cr.begin(), cr.end() ) == std::vector{ 1u, 2u, 3u, 4u, 5u, 6u, 7u, 9u } );
}
