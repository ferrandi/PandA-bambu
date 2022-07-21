
#ifndef PARLAY_BINARY_SEARCH_H_
#define PARLAY_BINARY_SEARCH_H_

#include <cstddef>

namespace parlay {
namespace internal {
  
// the following parameter can be tuned
constexpr const size_t _binary_search_base = 16;

template <typename Seq, typename F>
size_t linear_search(Seq const &I, typename Seq::value_type const &v,
                     const F &less) {
  for (size_t i = 0; i < I.size(); i++)
    if (!less(I[i], v)) return i;
  return I.size();
}

// return index to first key greater or equal to v
template <typename Seq, typename F>
size_t binary_search(Seq const &I, typename Seq::value_type const &v,
                     const F &less) {
  size_t start = 0;
  size_t end = I.size();
  while (end - start > _binary_search_base) {
    size_t mid = (end + start) / 2;
    if (!less(I[mid], v))
      end = mid;
    else
      start = mid + 1;
  }
  return start + linear_search(make_slice(I).cut(start, end), v, less);
}

template <typename Seq, typename F>
size_t linear_search(Seq const &I, const F &less) {
  for (size_t i = 0; i < I.size(); i++)
    if (!less(I[i])) return i;
  return I.size();
}

// return index to first key where less is false
template <typename Seq, typename F>
size_t binary_search(Seq const &I, const F &less) {
  size_t start = 0;
  size_t end = I.size();
  while (end - start > _binary_search_base) {
    size_t mid = (end + start) / 2;
    if (!less(I[mid]))
      end = mid;
    else
      start = mid + 1;
  }
  return start + linear_search(make_slice(I).cut(start, end), less);
}

}  // namespace internal
}  // namespace parlay

#endif  // PARLAY_BINARY_SEARCH_H_
