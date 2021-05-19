#include "cplusplus/cuddObj.hh"
#include "cudd/cuddInt.h"
#include <vector>
#include <string>
#include <iostream>
#include <sstream>
#include <unordered_map>
#include <fmt/format.h>

namespace cudd {

class cudd_zdd 
{
public:
  cudd_zdd( uint32_t num_variables ) 
    : num_variables( num_variables ), empty( cudd.zddZero() ), base( cudd.zddOne( INT_MAX ) )
  {
    elementaries.reserve( num_variables );
    for ( auto i = 0u; i < num_variables; ++i )
    {
      elementaries.emplace_back( base.Change( i ) );
    }
    tautologies.reserve( num_variables );
    for ( auto i = 0u; i < num_variables; ++i )
    {
      tautologies.emplace_back( cudd.zddOne( i ) );
    }
    assert( cudd.ReadZddSize() == int( num_variables ) );
    assert( Cudd_DebugCheck( cudd.getManager() ) == 0 );
  }

  ~cudd_zdd()
  {
  }

  void print( ZDD const& node, std::string const& name = "", int verbosity = 4 )
  {
    std::cout << name << " (" << node.getNode() << ", level = " << Cudd_NodeReadIndex( node.getNode() ) << ")";
    node.print( num_variables, verbosity );
  }

  ZDD ref( ZDD const& node )
  {
    Cudd_Ref( node.getNode() );
    return node;
  }

  void deref_rec( ZDD const& node )
  {
    Cudd_RecursiveDerefZdd( cudd.getManager(), node.getNode() );
    assert( Cudd_DebugCheck( cudd.getManager() ) == 0 );
  }

  /* only decrease ref count of `node` but not recursively on its children */
  void deref( ZDD const& node )
  {
    Cudd_Deref( node.getNode() );
  }

  /* Create a node at level `var`, whose "then" child points to `T` and "else" child points to `E`,
   * or return the node if it already exists.
   * The result ZDD object is pass by copy, hence ref count is incresed by the copy constructor.
   * 
   * `T` and `E` should be lower than `var` (larger indices are lower)
   * Note the order of `T` and `E` is the same as in `bill`, but different from CUDD.
   */
  ZDD unique( uint32_t var, ZDD const& E, ZDD const& T )
  {
    assert( T.NodeReadIndex() > var && E.NodeReadIndex() > var );
    return ZDD( cudd, cuddZddGetNode( cudd.getManager(), var, T.getNode(), E.getNode() ) );
  }

public: /* basic getters; does NOT increase ref count */
  ZDD& bottom() { return empty; }
  ZDD& top() { return base; }

  ZDD& elementary( uint32_t var )
  {
    assert( var < num_variables );
    return elementaries[var];
  }

  ZDD& tautology( uint32_t var = 0 )
  {
    assert( var < num_variables );
    return tautologies[var];
  }

public: /* operations provided by CUDD, wrapped with `bill` function names */
  ZDD union_( ZDD const& f, ZDD const& g )
  {
    return f.Union( g );
  }

  ZDD intersection( ZDD const& f, ZDD const& g )
  {
    return f.Intersect( g );
  }

  ZDD difference( ZDD const& f, ZDD const& g )
  {
    return f.Diff( g );
  }

public: /* operations not provided by CUDD */
  /* union every pair of subsets in f and g */
  ZDD join( ZDD const& f, ZDD const& g )
  {
    assert( ( f == empty || f == base || f.NodeReadIndex() < num_variables) && ( g == empty || g == base || g.NodeReadIndex() < num_variables ) );
    auto r = ZDD( cudd, join( f.getNode(), g.getNode() ) );
    assert( Cudd_DebugCheck( cudd.getManager() ) == 0 );
    return r;
  }

  /* resulting sets are elements in f, but not superset of any element in g */
  /* \forall A \in result, A \in f and \forall B \in g, A \notsuperset B */
  ZDD nonsupersets( ZDD const& f, ZDD const& g )
  {
    assert( ( f == empty || f == base || f.NodeReadIndex() < num_variables) && ( g == empty || g == base || g.NodeReadIndex() < num_variables ) );
    auto r = ZDD( cudd, nonsupersets( f.getNode(), g.getNode() ) );
    assert( Cudd_DebugCheck( cudd.getManager() ) == 0 );
    return r;
  }

  ZDD choose( ZDD const& f, uint32_t k )
  {
    assert( f == empty || f == base || f.NodeReadIndex() < num_variables );
    auto r = ZDD( cudd, choose( f.getNode(), k ) );
    assert( Cudd_DebugCheck( cudd.getManager() ) == 0 );
    return r;
  }

  /* builds an ZDD with one minterm, having '-' at variables in `vec`, and '0' at other variables
   * `vec` is assumed to be sorted decreasingly
   */
  ZDD care( std::vector<uint32_t> const& vec )
  {
    DdNode* n1 = base.getNode();
    DdNode* n2 = 0;

    for ( auto i : vec )
    {
      assert( i < num_variables );
      n2 = unique( i, n1, n1 ); ref( n2 );
      if ( n1 != base.getNode() ) { Cudd_Deref( n1 ); }
      n1 = n2;
    }

    Cudd_Deref( n2 );
    auto r = ZDD( cudd, n2 );
    assert( Cudd_DebugCheck( cudd.getManager() ) == 0 );
    return r;
  }

  /* inverse of care */
  ZDD dont_care( std::vector<uint32_t> const& vec )
  {
    std::vector<uint32_t> vec2;
    for ( int i = num_variables - 1; i >= 0; --i )
    {
      bool found = false;
      for ( auto j : vec )
      {
        if ( j == (unsigned)i )
        {
          found = true;
          break;
        }
      }
      if ( !found )
      {
        vec2.emplace_back( i );
      } 
    }

    return care( vec2 );
  }

  //edivide
  //maximal
  //meet
  //nonsubsets

private: /* implementation details */
  DdNode* lo( DdNode* f ) { return cuddE( f ); }
  DdNode* hi( DdNode* f ) { return cuddT( f ); }
  DdNode* ref( DdNode* node ) { Cudd_Ref( node ); return node; }
  DdNode* deref( DdNode* node )
  {
    cuddSatDec( node->ref );
    if ( node->ref == 0 )
    {
      DdManager * table = cudd.getManager();
      table->deadZ++;
      #ifdef DD_STATS
        table->nodesDropped++;
      #endif
      assert( !cuddIsConstant( node ) );
      int ord = table->permZ[node->index];
      table->subtableZ[ord].dead++;
      if ( !cuddIsConstant( cuddT( node ) ) ) { assert( cuddT( node )->ref != 0 ); Cudd_RecursiveDerefZdd( table, cuddT( node ) ); }
      if ( !cuddIsConstant( cuddE( node ) ) ) { assert( cuddE( node )->ref != 0 ); Cudd_RecursiveDerefZdd( table, cuddE( node ) ); }
    }
    return node; 
  }

  DdNode* unique( uint32_t var, DdNode* lo, DdNode* hi )
  { return cuddZddGetNode( cudd.getManager(), var, hi, lo ); }

  DdNode* union_( DdNode* f, DdNode* g )
  { return Cudd_zddUnion( cudd.getManager(), f, g ); }

  DdNode* intersection( DdNode* f, DdNode* g )
  { return Cudd_zddIntersect( cudd.getManager(), f, g ); }

  DdNode* difference( DdNode* f, DdNode* g )
  { return Cudd_zddDiff( cudd.getManager(), f, g ); }

  /* replacement of function pointers to be used in the operation cache */
  uint64_t join_ = 2;
  uint64_t nonsupersets_ = 4;
  uint64_t choose_ = 6;

  DdNode* join( DdNode* f, DdNode* g )
  {
    /* terminal cases */
    DdNode* e = empty.getNode();
    DdNode* b = base.getNode();
    if ( f == e || g == e ) { return e; }
    if ( f == b ) { return g; }
    if ( g == b ) { return f; }

    if ( Cudd_NodeReadIndex( f ) < Cudd_NodeReadIndex( g ) ) { return join( g, f ); }

    /* cache lookup */
    auto res = cache_lookup( cudd.getManager(), join_, f, g );
    if ( res != NULL ) { return res; }

    uint32_t var = Cudd_NodeReadIndex( g );
    DdNode* lo = 0;
    DdNode* hi = 0;
    
    if ( Cudd_NodeReadIndex( f ) > Cudd_NodeReadIndex( g ) )
    {
      lo = join( f, cuddE( g ) ); ref( lo );
      hi = join( f, cuddT( g ) ); ref( hi );
    }
    else /* f_var == g_var */
    {
      #if 1 /* Knuth's book */
      DdNode* tmp0 = join( cuddE( f ), cuddT( g ) ); ref( tmp0 );
      DdNode* tmp1 = join( cuddT( f ), cuddE( g ) ); ref( tmp1 );
      DdNode* tmp2 = join( cuddT( f ), cuddT( g ) ); ref( tmp2 );
      DdNode* tmp3 = union_( tmp0, tmp1 ); ref( tmp3 );
      deref( tmp0 ); deref( tmp1 );
      hi = union_( tmp2, tmp3 ); ref( hi );
      deref( tmp2 ); deref( tmp3 );
      #else /* Bruno's implementation */
      DdNode* tmp0 = union_( cuddE( g ), cuddT( g ) ); ref( tmp0 );
      DdNode* tmp1 = join( cuddT( f ), tmp0 ); ref( tmp1 );
      deref( tmp0 );
      DdNode* tmp2 = join( cuddE( f ), cuddT( g ) ); ref( tmp2 );
      hi = union_( tmp1, tmp2 ); ref( hi );
      deref( tmp1 ); deref( tmp2 );
      #endif
      lo = join( cuddE( f ), cuddE( g ) ); ref( lo );
    }
    auto r = unique( var, lo, hi );
    deref( lo ); deref( hi );
    
    cache_insert( cudd.getManager(), join_, f, g, r );
    return r;
  }

  DdNode* nonsupersets( DdNode* f, DdNode* g )
  {
    /* terminal cases */
    DdNode* e = empty.getNode();
    DdNode* b = base.getNode();
    if ( g == e ) { return f; }
    if ( f == e || g == b || f == g ) { return e; }

    if ( Cudd_NodeReadIndex( f ) > Cudd_NodeReadIndex( g ) )
      { return nonsupersets( f, cuddE( g ) ); }

    /* cache lookup */
    auto res = cache_lookup( cudd.getManager(), nonsupersets_, f, g );
    if ( res != NULL ) { return res; }

    /* recursive computation */
    DdNode* lo = 0;
    DdNode* hi = 0;
    if ( Cudd_NodeReadIndex( f ) < Cudd_NodeReadIndex( g ) )
    {
      lo = nonsupersets( cuddE( f ), g ); ref( lo );
      hi = nonsupersets( cuddT( f ), g ); ref( hi );
    }
    else /* f_var == g_var */
    {
      DdNode* tmp0 = nonsupersets( cuddT( f ), cuddT( g ) ); ref( tmp0 );
      DdNode* tmp1 = nonsupersets( cuddT( f ), cuddE( g ) ); ref( tmp1 );
      hi = intersection( tmp0, tmp1 ); ref( hi );
      deref( tmp0 ); deref( tmp1 );
      lo = nonsupersets( cuddE( f ), cuddE( g ) ); ref( lo );
    }
    auto r = unique( Cudd_NodeReadIndex( f ), lo, hi );
    //deref( lo ); deref( hi );
    
    cache_insert( cudd.getManager(), nonsupersets_, f, g, r );
    return r;
  }

  DdNode* choose( DdNode* f, uint64_t k )
  {
    if ( Cudd_NodeReadIndex( f ) >= num_variables )
    {
      return k > 0 ? empty.getNode() : base.getNode();
    }
    if ( k == 1 ) { return f; }
    if ( k == 0 ) { return choose( cuddE( f ), k ); }

    /* cache lookup */
    auto res = cache_lookup( cudd.getManager(), choose_ + k * 2, f, f );
    if ( res != NULL ) { return res; }

    /* k > 0 */
    DdNode* n = choose( cuddE( f ), k ); ref( n ); /* don't take this var */
    DdNode* tmp = choose( cuddE( f ), k - 1 ); ref( tmp ); /* take this var */
    auto r = unique( Cudd_NodeReadIndex( f ), n, tmp );
    deref( n ); deref( tmp );

    cache_insert( cudd.getManager(), choose_ + k * 2, f, f, r );
    return r;
  }

private: /* iterator, counting, etc */
  template<class Fn>
  bool foreach_set_rec( DdNode* f, std::vector<uint32_t>& set, Fn&& fn ) const
  {
    if ( f == base.getNode() )
    {
      return fn(set);
    }
    if ( f != empty.getNode() )
    {
      if ( !foreach_set_rec( cuddE( f ), set, fn ) )
      {
        return false;
      }
      auto new_set = set;
      new_set.push_back( Cudd_NodeReadIndex( f ) );
      if ( !foreach_set_rec( cuddT( f ), new_set, fn ) )
      {
        return false;
      }
    }
    return true;
  }

  uint64_t count_sets_rec( DdNode* f, std::unordered_map<DdNode*, uint64_t>& visited ) const
  {
    if ( f == base.getNode() )
    {
      return 1;
    }
    if ( f == empty.getNode() )
    {
      return 0;
    }

    const auto it = visited.find( f );
    if ( it != visited.end() )
    {
      return it->second;
    }
    return visited[f] = count_sets_rec( cuddE( f ), visited ) + count_sets_rec( cuddT( f ), visited );
  }

public:
  /* a set is represented with a `std::vector<uint32_t>` of variable indices */
  template<class Fn>
  void foreach_set( ZDD const& f, Fn&& fn ) const
  {
    std::vector<uint32_t> set;
    foreach_set_rec( f.getNode(), set, fn );
  }

  void print_sets( ZDD const& f, std::ostream& os = std::cout ) const
  {
    foreach_set( f, [&]( auto const& set ){
      os << fmt::format("{{ {} }}\n", fmt::join( set, ", " ) );
      return true;
    });
  }

  /* \!brief Return the number of nodes in a ZDD. */
  uint64_t count_nodes( DdNode* f ) const
  {
    return Cudd_zddDagSize(f);
  }

  /* \!brief Return the number of sets in a ZDD. */
  uint64_t count_sets( ZDD const& f ) const
  {
    if ( f.getNode() == base.getNode() )
    {
      return 1;
    }
    if ( f.getNode() == empty.getNode() )
    {
      return 0;
    }
    std::unordered_map<DdNode*, uint64_t> visited;
    return count_sets_rec( f.getNode(), visited );
  }

  std::vector<std::vector<uint32_t>> sets_as_vectors( ZDD const& f ) const
  {
    std::vector<std::vector<uint32_t>> sets_vectors;
    foreach_set( f, [&]( auto const& set ){
      sets_vectors.emplace_back( set );
      return true;
    });
    return sets_vectors;
  }

private: /* operation cache */
  DdNode * cache_lookup( DdManager * table, uint64_t op, DdNode * f, DdNode * g )
  {
    int posn;
    DdCache *en,*cache;
    DdNode *data;

    cache = table->cache;
    #ifdef DD_DEBUG
      if (cache == NULL) {
        return(NULL);
      }
    #endif

    posn = ddCHash2(op,f,g,table->cacheShift);
    en = &cache[posn];
    if (en->data != NULL && en->f==f && en->g==g && en->h==(ptruint)op) {
      data = Cudd_Regular(en->data);
      table->cacheHits++;
      if (data->ref == 0) {
        cuddReclaimZdd(table,data);
      }
      return(en->data);
    }

    /* Cache miss: decide whether to resize. */
    table->cacheMisses++;

    if (table->cacheSlack >= 0 && table->cacheHits > table->cacheMisses * table->minHit) {
      cuddCacheResize(table);
    }

    return(NULL);
  }

  void cache_insert( DdManager * table, uint64_t op, DdNode * f, DdNode * g, DdNode * data )
  {
    int posn;
    DdCache *entry;

    posn = ddCHash2(op,f,g,table->cacheShift);
    entry = &table->cache[posn];

    if (entry->data != NULL) {
      table->cachecollisions++;
    }
    table->cacheinserts++;

    entry->f = f;
    entry->g = g;
    entry->h = (ptruint) op;
    entry->data = data;
    #ifdef DD_CACHE_PROFILE
      entry->count++;
    #endif
  } 

private:
  Cudd cudd; /* the CUDD manager */
  uint32_t num_variables;
  ZDD empty; /* the empty family {} (minterms: none; constant 0) */
  ZDD base; /* the unit family {{}} (minterms: the all-zero cube) */
  std::vector<ZDD> elementaries; /* the single-set family of the single-element set {{i}} (minterms: 0...010...0) */
  std::vector<ZDD> tautologies; /* every combinations of variables >= var (minterms: 0...0-...-) */
};

} // namespace cudd