// This file is a bridge connecting the "lib interface" gbbs exports and the
// interface that the current parlaylib exports.

#pragma once

#include <type_traits>
#include <utility>

#include "parlay/delayed_sequence.h"
#include "parlay/internal/binary_search.h"
#include "parlay/internal/get_time.h"
#include "parlay/io.h"
#include "parlay/monoid.h"
#include "parlay/parallel.h"
#include "parlay/primitives.h"
#include "parlay/random.h"
#include "parlay/range.h"
#include "parlay/sequence.h"
#include "parlay/slice.h"
#include "parlay/utilities.h"

namespace gbbs {
// ================== parallel primitives ===================

using parlay::parallel_for;
using parlay::par_do;
using parlay::num_workers;
using parlay::worker_id;

// parallel loop from start (inclusive) to end (exclusive) running
// function f.
//    f should map long to void.
//    granularity is the number of iterations to run sequentially
//      if 0 (default) then the scheduler will decide
//    conservative uses a safer scheduler
template <typename F>
inline void parallel_for(size_t start, size_t end, long granularity, F f,
                         bool conservative = false) {
  return parallel_for(start, end, f, granularity, conservative);
}

template <typename A, typename Af, typename Df, typename F>
static void parallel_for_alloc(Af init_alloc, Df finish_alloc, long start,
                               long end, F f, long granularity = 0,
                               bool conservative = false);

#ifdef CILK
// TODO try parallel_for_1
template <typename A, typename Af, typename Df, typename F>
inline void parallel_for_alloc(Af init_alloc, Df finish_alloc, long start,
                               long end, F f, long granularity,
                               bool conservative) {
  alloc_holder<A> alloc;
  parallel_for_1(start, end,
                 [&](size_t i) {
                   init_alloc(&alloc.imp_.view());
                   f(i, &(alloc.imp_.view()));
                   // finish_alloc(&(alloc.imp_.view()));
                 },
                 granularity, conservative);
}
#else
#ifdef OPENMP
template <typename A, typename Af, typename Df, typename F>
inline void parallel_for_alloc(Af init_alloc, Df finish_alloc, long start,
                               long end, F f, long granularity,
                               bool conservative) {
  A* alloc = nullptr;
#pragma omp parallel private(alloc)
  {
    alloc = new A();
    init_alloc(alloc);
    parallel_for_1(start, end, [&](size_t i) { f(i, alloc); }, granularity,
                   conservative);
    //#pragma omp for schedule(dynamic, 1) nowait
    // for(long i=start; i<end; i++) f(i, alloc);
    finish_alloc(alloc);
  }
}
#else
template <typename A, typename Af, typename Df, typename F>
inline void parallel_for_alloc(Af init_alloc, Df finish_alloc, long start,
                               long end, F f, long granularity,
                               bool conservative) {
  parallel_for(start, end,
               [&](long i) {
                 static thread_local A* alloc = new A();
                 init_alloc(alloc);
                 f(i, alloc);
               },
               granularity, conservative);
  // finish_alloc(alloc);
}
#endif
#endif

template <class E>
E* new_array_no_init(size_t n) {
#ifndef PARLAY_USE_STD_ALLOC
  auto allocator = parlay::allocator<E>();
#else
  auto allocator = std::allocator<E>();
#endif
  return allocator.allocate(n);
}

// Initializes in parallel
template <typename E>
E* new_array(size_t n) {
  E* r = new_array_no_init<E>(n);
  if (!std::is_trivially_default_constructible<E>::value) {
    // if (!std::is_default_constructible<E>::value) {
    if (n > 2048) {
      auto f = [&](size_t i) { new ((void*)(r + i)) E; };
      parallel_for(0, n, f);
    } else
      for (size_t i = 0; i < n; i++) new ((void*)(r + i)) E;
  }
  return r;
}

template <class E>
void free_array(E* e, size_t n) {
#ifndef PARLAY_USE_STD_ALLOC
  auto allocator = parlay::allocator<E>();
#else
  auto allocator = std::allocator<E>();
#endif
  allocator.deallocate(e, n);
}

// Alias template for parlay::sequence
template <typename T>
using sequence = parlay::sequence<T>;

// Bit shorter than writing slice<T*, T*> everywhere.
template <class T>
using slice = parlay::slice<T*, T*>;

template <typename Seq>
auto make_slice(const Seq& S) {
  return parlay::make_slice(S.begin(), S.end());
}

template <class E>
slice<E> make_slice(E* start, E* end) {
  return parlay::make_slice((E*)start, (E*)end);
}

template <class T>
inline slice<T> make_slice(T* A, size_t n) {
  return parlay::make_slice((T*)A, (T*)(A + n));
}

struct empty {
};  // struct containing no data (used for empty base optimization)

// ========================= timer  ==========================
using parlay::internal::timer;

// ========================= atomic ops  ==========================

// Currently unused, but may be useful in the future; including commented out.
// template <class ET>
// inline bool CAS128(ET* a, ET b, ET c) {
//   return __sync_bool_compare_and_swap_16((__int128*)a, *((__int128*)&b),
//                                          *((__int128*)&c));
// }

template <typename ET>
inline ET atomic_load(ET* a) {
  ET tmp;
  __atomic_load(a, &tmp, __ATOMIC_RELAXED);
  return tmp;
}

template <typename ET>
inline void atomic_store(ET* a, ET b) {
  __atomic_store(a, &b, __ATOMIC_RELAXED);
}

template <typename ET>
inline bool atomic_compare_and_swap(ET* a, ET oldval, ET newval) {
  if
    constexpr(sizeof(ET) == 1) {
      uint8_t r_oval, r_nval;
      std::memcpy(&r_oval, &oldval, sizeof(ET));
      std::memcpy(&r_nval, &newval, sizeof(ET));
      return __sync_bool_compare_and_swap(reinterpret_cast<uint8_t*>(a), r_oval,
                                          r_nval);
    }
  else if
    constexpr(sizeof(ET) == 4) {
      uint32_t r_oval, r_nval;
      std::memcpy(&r_oval, &oldval, sizeof(ET));
      std::memcpy(&r_nval, &newval, sizeof(ET));
      return __sync_bool_compare_and_swap(reinterpret_cast<uint32_t*>(a),
                                          r_oval, r_nval);
    }
  else if
    constexpr(sizeof(ET) == 8) {
      uint64_t r_oval, r_nval;
      std::memcpy(&r_oval, &oldval, sizeof(ET));
      std::memcpy(&r_nval, &newval, sizeof(ET));
      return __sync_bool_compare_and_swap(reinterpret_cast<uint64_t*>(a),
                                          r_oval, r_nval);
    }
  else if
    constexpr(sizeof(ET) == 16) {
      __int128 r_oval, r_nval;
      std::memcpy(&r_oval, &oldval, sizeof(ET));
      std::memcpy(&r_nval, &newval, sizeof(ET));
      return __sync_bool_compare_and_swap_16(reinterpret_cast<__int128*>(a),
                                             r_oval, r_nval);
    }
  else {
    std::cout << "Bad CAS Length" << sizeof(ET) << std::endl;
    exit(0);
  }
}

template <typename ET>
inline bool atomic_compare_and_swap(volatile ET* a, ET oldval, ET newval) {
  if (sizeof(ET) == 1) {
    uint8_t r_oval, r_nval;
    std::memcpy(&r_oval, &oldval, sizeof(ET));
    std::memcpy(&r_nval, &newval, sizeof(ET));
    return __sync_bool_compare_and_swap(reinterpret_cast<volatile uint8_t*>(a),
                                        r_oval, r_nval);
  } else if (sizeof(ET) == 4) {
    uint32_t r_oval, r_nval;
    std::memcpy(&r_oval, &oldval, sizeof(ET));
    std::memcpy(&r_nval, &newval, sizeof(ET));
    return __sync_bool_compare_and_swap(reinterpret_cast<volatile uint32_t*>(a),
                                        r_oval, r_nval);
  } else if (sizeof(ET) == 8) {
    uint64_t r_oval, r_nval;
    std::memcpy(&r_oval, &oldval, sizeof(ET));
    std::memcpy(&r_nval, &newval, sizeof(ET));
    return __sync_bool_compare_and_swap(reinterpret_cast<volatile uint64_t*>(a),
                                        r_oval, r_nval);
  } else if (sizeof(ET) == 16) {
    __int128 r_oval, r_nval;
    std::memcpy(&r_oval, &oldval, sizeof(ET));
    std::memcpy(&r_nval, &newval, sizeof(ET));
    return __sync_bool_compare_and_swap_16(
        reinterpret_cast<volatile __int128*>(a), r_oval, r_nval);
  } else {
    std::cout << "Bad CAS Length" << sizeof(ET) << std::endl;
    exit(0);
  }
}

template <typename E, typename EV>
inline E fetch_and_add(E* a, EV b) {
  volatile E newV, oldV;
  do {
    oldV = *a;
    newV = oldV + b;
  } while (!atomic_compare_and_swap(a, oldV, newV));
  return oldV;
}

// Atomically increment *a by b unless the value is larger than max_v.
// If the increment is successful, it returns an opption containing
// the previous value and returns an empty std::optional<E> otherwise.
template <typename E, typename EV>
inline std::optional<E> fetch_and_add_threshold(E* a, EV b, EV max_v) {
  volatile E newV, oldV;
  oldV = *a;
  newV = oldV + b;
  while (oldV <= max_v) {
    if (atomic_compare_and_swap(a, oldV, newV)) return oldV;
    oldV = atomic_load(a);
    newV = oldV + b;
  }
  return std::nullopt;
}

template <typename E, typename EV>
inline void write_add(E* a, EV b) {
  // volatile E newV, oldV;
  E newV, oldV;
  do {
    oldV = *a;
    newV = oldV + b;
  } while (!atomic_compare_and_swap(a, oldV, newV));
}

template <typename E, typename EV>
inline void write_add(std::atomic<E>* a, EV b) {
  // volatile E newV, oldV;
  E newV, oldV;
  do {
    oldV = a->load();
    newV = oldV + b;
  } while (!std::atomic_compare_exchange_strong(a, &oldV, newV));
}

template <typename ET, typename F>
inline bool write_min(ET* a, ET b, F less) {
  ET c;
  bool r = 0;
  do
    c = *a;
  while (less(b, c) && !(r = atomic_compare_and_swap(a, c, b)));
  return r;
}

template <typename ET, typename F>
inline bool write_min(volatile ET* a, ET b, F less) {
  ET c;
  bool r = 0;
  do
    c = *a;
  while (less(b, c) && !(r = atomic_compare_and_swap(a, c, b)));
  return r;
}

template <typename ET, typename F>
inline bool write_min(std::atomic<ET>* a, ET b, F less) {
  ET c;
  bool r = 0;
  do
    c = a->load();
  while (less(b, c) && !(r = std::atomic_compare_exchange_strong(a, &c, b)));
  return r;
}

template <typename ET, typename F>
inline bool write_max(ET* a, ET b, F less) {
  ET c;
  bool r = 0;
  do
    c = *a;
  while (less(c, b) && !(r = atomic_compare_and_swap(a, c, b)));
  return r;
}

template <typename ET, typename F>
inline bool write_max(volatile ET* a, ET b, F less) {
  ET c;
  bool r = 0;
  do
    c = *a;
  while (less(c, b) && !(r = atomic_compare_and_swap(a, c, b)));
  return r;
}

template <typename ET, typename F>
inline bool write_max(std::atomic<ET>* a, ET b, F less) {
  ET c;
  bool r = 0;
  do
    c = a->load();
  while (less(c, b) && !(r = std::atomic_compare_exchange_strong(a, &c, b)));
  return r;
}

template <typename ET>
inline bool CAS(ET* ptr, const ET oldv, const ET newv) {
  return atomic_compare_and_swap(ptr, oldv, newv);
}

template <typename ET>
inline bool write_min(ET* a, ET b) {
  return write_min<ET>(a, b, std::less<ET>());
}

template <typename ET>
inline bool write_max(ET* a, ET b) {
  return write_max<ET>(a, b, std::less<ET>());
}

// Combines two hash values.
inline uint64_t hash_combine(uint64_t hash_value_1, uint64_t hash_value_2) {
  // This is the same as boost's 32-bit `hash_combine` implementation, but with
  // 2 ^ 64 / (golden ratio) chosen as an arbitrary 64-bit additive magic number
  // rather than 2 ^ 32 / (golden ratio).
  return hash_value_1 ^ (hash_value_2 + 0x9e3779b97f4a7c15 +
                         (hash_value_1 << 6) + (hash_value_1 >> 2));
}

template <class E, class I, class P>
struct filter_iter {
  I& iter;
  P& pred;
  E cur_val;

  filter_iter(I& _it, P& _pr) : iter(_it), pred(_pr) {
    cur_val = iter.cur();
    while (!pred(cur_val) && iter.has_next()) {
      cur_val = iter.next();
    }
  }

  E cur() { return cur_val; }

  E next() {
    while (iter.has_next()) {
      cur_val = iter.next();
      if (pred(cur_val)) {
        break;
      }
    }
    return cur_val;
  }
  // has_next
};

template <class E, class I, class P>
inline filter_iter<E, I, P> make_filter_iter(I& _it, P& _pr) {
  return filter_iter<E, I, P>(_it, _pr);
}

}  // namespace gbbs

namespace parlay {

template <class Seq>
inline auto reduce_max(Seq const& I) -> typename Seq::value_type {
  using T = typename Seq::value_type;
  return reduce(make_slice(I), maxm<T>());
}

template <class Seq>
inline auto reduce_min(Seq const& I) -> typename Seq::value_type {
  using T = typename Seq::value_type;
  return reduce(make_slice(I), minm<T>());
}

template <class Seq>
inline auto reduce_xor(Seq const& I) -> typename Seq::value_type {
  using T = typename Seq::value_type;
  return reduce(make_slice(I), xorm<T>());
}

using parlay::internal::sample_sort;
using parlay::internal::sample_sort_inplace;
using parlay::internal::pack_out;

using parlay::internal::filter_out;
using parlay::internal::split_two;

constexpr const size_t _log_block_size = 10;
constexpr const size_t _block_size = (1 << _log_block_size);

inline size_t num_blocks(size_t n, size_t block_size) {
  if (n == 0)
    return 0;
  else
    return (1 + ((n)-1) / (block_size));
}

// Writes the list of indices `i` where `Fl[i] == true` to range `Out`.
template <class Bool_Seq, class Out_Seq>
size_t pack_index_out(Bool_Seq const& Fl, Out_Seq&& Out, flags fl = no_flag) {
  using Idx_Type = typename std::remove_reference<Out_Seq>::type::value_type;
  auto identity = [](size_t i) { return (Idx_Type)i; };
  return pack_out(delayed_seq<Idx_Type>(Fl.size(), identity), Fl,
                  std::forward<Out_Seq>(Out), fl);
}

using parlay::internal::binary_search;

constexpr size_t _F_BSIZE = 2000;

// Transforms input sequence `[a_0, a_1, ..., a_{n-1}]` to sequence `[f(0, a_0),
// f(1, a_1), ..., f(n-1, a_{n-1})]` using input function `f`.
//
// Arguments:
//   A: sequence-like object with elements of type `T`
//     Input array.
//   f: (size_t, T) -> OT
//     Function to apply to input array.
//
// Returns:
//   sequence<OT>
//     Result of applying `f` to each element of `A` along with the index of
//     that element in `A`.
template <class OT, class Seq, class Func>
auto map_with_index(Seq const& A, Func&& f, flags fl = no_flag)
    -> sequence<OT> {
  return sequence<OT>::from_function(A.size(),
                                     [&](size_t i) { return f(i, A[i]); });
}

template <class OT, class Seq, class UnaryFunc>
auto map(Seq const& A, UnaryFunc f, flags fl = no_flag) -> sequence<OT> {
  return sequence<OT>::from_function(A.size(),
                                     [&](size_t i) { return f(A[i]); });
}

template <class In_Seq, class F>
auto filter_index(In_Seq const& In, F f, flags fl = no_flag)
    -> sequence<typename In_Seq::value_type> {
  using T = typename In_Seq::value_type;
  size_t n = In.size();
  size_t l = num_blocks(n, _block_size);
  sequence<size_t> Sums(l);
  sequence<bool> Fl(n);
  parlay::internal::sliced_for(n, _block_size,
                               [&](size_t i, size_t s, size_t e) {
                                 size_t r = 0;
                                 for (size_t j = s; j < e; j++)
                                   r += (Fl[j] = f(In[j], j));
                                 Sums[i] = r;
                               });
  size_t m = parlay::scan_inplace(make_slice(Sums));
  sequence<T> Out = sequence<T>::uninitialized(m);
  parlay::internal::sliced_for(
      n, _block_size, [&](size_t i, size_t s, size_t e) {
        parlay::internal::pack_serial_at(
            make_slice(In).cut(s, e), make_slice(Fl).cut(s, e),
            make_slice(Out).cut(Sums[i], (i == l - 1) ? m : Sums[i + 1]));
      });
  return Out;
}

template <class Idx_Type, class D, class F>
inline sequence<std::tuple<Idx_Type, D> > pack_index_and_data(F& f,
                                                              size_t size) {
  auto id_seq =
      parlay::delayed_seq<std::tuple<Idx_Type, D> >(size, [&](size_t i) {
        return std::make_tuple((Idx_Type)i, std::get<1>(f[i]));
      });
  auto flgs_seq = parlay::delayed_seq<bool>(
      size, [&](size_t i) { return std::get<0>(f[i]); });

  return parlay::pack(id_seq, flgs_seq);
}

template <class Seq, class Compare>
typename Seq::value_type kth_smallest(Seq const& s, size_t k, Compare less,
                                      random r = random()) {
  using T = typename Seq::value_type;
  size_t n = s.size();
  T pivot = s[r[0] % n];
  sequence<T> smaller = filter(s, [&](T a) { return less(a, pivot); });
  if (k < smaller.size())
    return kth_smallest(smaller, k, less, r.next());
  else {
    sequence<T> larger = filter(s, [&](T a) { return less(pivot, a); });
    if (k >= n - larger.size())
      return kth_smallest(larger, k - n + larger.size(), less, r.next());
    else
      return pivot;
  }
}

template <class Seq, class Compare>
typename Seq::value_type approximate_kth_smallest(Seq const& S, size_t k,
                                                  Compare less,
                                                  random r = random()) {
  // raise exception if empty sequence?
  using T = typename Seq::value_type;
  size_t n = S.size();
  size_t num_samples = n / sqrt(n);
  sequence<T> samples = sequence<T>::from_function(
      num_samples, [&](size_t i) -> T { return S[r[i] % n]; });
  return sample_sort(make_slice(samples), less)[k * num_samples / n];
}

template <class T, class Pred>
inline size_t filter_seq(T* in, T* out, size_t n, Pred p) {
  size_t k = 0;
  for (size_t i = 0; i < n; i++)
    if (p(in[i])) out[k++] = in[i];
  return k;
}

// Faster for a small number in output (about 40% or less)
// Destroys the input.   Does not need a bool array.
template <class T, class PRED>
inline size_t filterf(T* In, T* Out, size_t n, PRED p) {
  size_t b = _F_BSIZE;
  if (n < b) return filter_seq(In, Out, n, p);
  size_t l = num_blocks(n, b);
  auto Sums = sequence<size_t>::uninitialized(l + 1);
  parallel_for(0, l,
               [&](size_t i) {
                 size_t s = i * b;
                 size_t e = std::min(s + b, n);
                 size_t k = s;
                 for (size_t j = s; j < e; j++) {
                   if (p(In[j])) In[k++] = In[j];
                 }
                 Sums[i] = k - s;
               },
               1);
  Sums[l] = 0;
  size_t m = parlay::scan_inplace(make_slice(Sums));
  Sums[l] = m;
  parallel_for(0, l,
               [&](size_t i) {
                 T* I = In + i * b;
                 T* O = Out + Sums[i];
                 for (size_t j = 0; j < Sums[i + 1] - Sums[i]; j++) {
                   O[j] = I[j];
                 }
               },
               1);
  return m;
}

// Faster for a small number in output (about 40% or less)
// Destroys the input.   Does not need a bool array.
template <class T, class PRED, class OUT>
inline size_t filterf(T* In, size_t n, PRED p, OUT out, size_t out_off) {
  size_t b = _F_BSIZE;
  if (n < b) {
    size_t k = out_off;
    for (size_t i = 0; i < n; i++) {
      if (p(In[i])) out(k++, In[i]);
    }
    return k - out_off;
  }
  size_t l = num_blocks(n, b);
  auto Sums = sequence<size_t>::uninitialized(l + 1);
  parallel_for(0, l,
               [&](size_t i) {
                 size_t s = i * b;
                 size_t e = std::min(s + b, n);
                 size_t k = s;
                 for (size_t j = s; j < e; j++) {
                   if (p(In[j])) In[k++] = In[j];
                 }
                 Sums[i] = k - s;
               },
               1);
  Sums[l] = 0;
  size_t m = parlay::scan_inplace(make_slice(Sums));
  Sums[l] = m;
  parallel_for(0, l,
               [&](size_t i) {
                 T* I = In + i * b;
                 size_t si = out_off + Sums[i];
                 for (size_t j = 0; j < Sums[i + 1] - Sums[i]; j++) {
                   out(si + j, I[j]);
                 }
               },
               1);
  return m;
}

// String utilities

inline int t_to_stringlen(long a) { return 21; }
inline void type_to_string(char* s, long a) { sprintf(s, "%ld", a); }
inline int t_to_stringlen(unsigned long a) { return 21; }
inline void type_to_string(char* s, unsigned long a) { sprintf(s, "%lu", a); }
inline uint t_to_stringlen(uint a) { return 12; }
inline void type_to_string(char* s, uint a) { sprintf(s, "%u", a); }
inline int t_to_stringlen(int a) { return 12; }
inline void type_to_string(char* s, int a) { sprintf(s, "%d", a); }
inline int t_to_stringlen(double a) { return 18; }
inline int t_to_stringlen(char* a) { return strlen(a) + 1; }
inline void type_to_string(char* s, char* a) { sprintf(s, "%s", a); }
inline void type_to_string(char* s, double a) { sprintf(s, "%.11le", a); }

template <class A, class B>
inline int t_to_stringlen(std::pair<A, B> a) {
  return t_to_stringlen(a.first) + t_to_stringlen(a.second) + 1;
}

template <class A, class B>
inline int t_to_stringlen(std::tuple<A, B> a) {
  return t_to_stringlen(std::get<0>(a)) + t_to_stringlen(std::get<1>(a)) + 1;
}

template <class A, class B, class C>
inline int t_to_stringlen(std::tuple<A, B, C> a) {
  return t_to_stringlen(std::get<0>(a)) + t_to_stringlen(std::get<1>(a)) +
         t_to_stringlen(std::get<2>(a)) + 2;
}

template <class A, class B>
inline void type_to_string(char* s, std::pair<A, B> a) {
  int l = t_to_stringlen(a.first);
  type_to_string(s, a.first);
  s[l] = ' ';
  type_to_string(s + l + 1, a.second);
}

template <class A, class B>
inline void type_to_string(char* s, std::tuple<A, B> a) {
  int l = t_to_stringlen(std::get<0>(a));
  type_to_string(s, std::get<0>(a));
  s[l] = ' ';
  type_to_string(s + l + 1, std::get<1>(a));
}

template <class A, class B, class C>
inline void type_to_string(char* s, std::tuple<A, B, C> a) {
  int l = t_to_stringlen(std::get<0>(a));
  type_to_string(s, std::get<0>(a));
  s[l] = ' ';
  int l1 = t_to_stringlen(std::get<1>(a));
  type_to_string(s + l + 1, std::get<1>(a));
  s[l + l1 + 1] = ' ';
  type_to_string(s + l + l1 + 2, std::get<2>(a));
}

template <class TSeq>
sequence<char> sequence_to_string(TSeq const& T) {
  size_t n = T.size();
  auto S = sequence<size_t>::from_function(n, [&](size_t i) {
    return t_to_stringlen(T[i]) + 1;  // +1 for \n
  });
  size_t m = parlay::scan_inplace(make_slice(S), addm<size_t>());

  auto C = sequence<char>::from_function(m, [&](size_t i) { return (char)0; });
  parallel_for(0, n - 1, [&](size_t i) {
    type_to_string(C.begin() + S[i], T[i]);
    C[S[i + 1] - 1] = '\n';
  });
  type_to_string(C.begin() + S[n - 1], T[n - 1]);
  C[m - 1] = '\n';

  return parlay::filter(make_slice(C), [&](char A) { return A > 0; });
}

using parlay::internal::chars_to_int_t;
using parlay::internal::get_counts;

}  // namespace parlay
