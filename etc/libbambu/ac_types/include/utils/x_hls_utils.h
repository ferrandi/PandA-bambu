/**
* Code taken from HLS4ML project.
* Licensed under the Apache License, Version 2.0 (the "License"). You may
* not use this file except in compliance with the License. A copy of the
* License is located at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
* WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
* License for the specific language governing permissions and limitations
* under the License.
*/

#ifndef X_HLS_UTILS_H
#define X_HLS_UTILS_H
#include "ap_fixed.h"
#include <limits>

namespace hls {

    template<typename T>
    class numeric_limits {
    public:
        static T max()     { return std::numeric_limits<T>::max(); }
        static T min()     { return std::numeric_limits<T>::min(); }
        static T epsilon() { return std::numeric_limits<T>::epsilon(); }
    };

    template <int W, int I, ap_q_mode Q, ap_o_mode O>
    class numeric_limits<ap_fixed<W,I,Q,O> > {
    public:
        static ap_fixed<W,I,Q,O> max() {
            ap_int<W> m = ::hls::numeric_limits<ap_int<W> >::max();
            ap_fixed<W,I,Q,O> x;
            x(W-1,0) = m(W-1,0);
            return x;
        }
        static ap_fixed<W,I,Q,O> min() {
            ap_int<W> m = ::hls::numeric_limits<ap_int<W> >::min();
            ap_fixed<W,I,Q,O> x;
            x(W-1,0) = m(W-1,0);
            return x;
        }
        static ap_fixed<W,I,Q,O> epsilon() {
          ap_fixed<W,I,Q,O> x = 0;
          x[0] = 1;
          return x;
        }
    };

    template <int W, int I, ap_q_mode Q, ap_o_mode O>
    class numeric_limits<ap_ufixed<W,I,Q,O> > {
    public:
        static ap_ufixed<W,I,Q,O> max() {
            ap_uint<W> m = ::hls::numeric_limits<ap_uint<W> >::max();
            ap_ufixed<W,I,Q,O> x;
            x(W-1,0) = m(W-1,0);
            return x;
        }
        static ap_ufixed<W,I,Q,O> min() { return 0; }
        static ap_ufixed<W,I,Q,O> epsilon() {
          ap_ufixed<W,I,Q,O> x = 0;
          x[0] = 1;
          return x;
        }
    };

    template <int W>
    class numeric_limits<ap_int<W> > {
    public:
        static ap_int<W> max() { ap_int<W> m = min(); return ~m; }
        static ap_int<W> min() { ap_int<W> m = 0; m[W-1] = 1; return m; }
        static ap_int<W> epsilon() {
          ap_int<W> x = 0;
          x[0] = 1;
          return x;
        }
    };

    template <int W>
    class numeric_limits<ap_uint<W> > {
    public:
        static ap_uint<W> max() { ap_uint<W> zero = 0; return ~zero; }
        static ap_uint<W> min() { return 0; }
        static ap_uint<W> epsilon() {
          ap_uint<W> x = 0;
          x[0] = 1;
          return x;
        }
    };
}

#endif
