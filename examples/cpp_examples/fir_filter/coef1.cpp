/// This code is an adaptation of the Shawn's DSP Tutorials. In particular, I started from this link.
/// https://sestevenson.wordpress.com/2009/10/08/implementation-of-fir-filtering-in-c-part-2/
/// The first relevant change is in the initialization of coeffs variable done by exploiting constexpr described by the c++14 standard.
/// The second change is on the number of samples that can be handled by a function call. Here we assume a single sample per function call.
/// @author Fabrizio Ferrandi - Politecnico di Milano

#include <stdio.h>
#include <stdint.h>
#define _USE_MATH_DEFINES
#include <array>    // array
#include <utility>  // index_sequence, make_index_sequence
#include <cmath>    // sinf,cosf
#include <cstddef>  // size_t
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <cassert>

//////////////////////////////////////////////////////////////
//  Filter Code Definitions
//////////////////////////////////////////////////////////////


template <int filter_len>
class firFixedClass
{
        constexpr static short int compute_coeff(size_t index)
        {
            // parameters and simulation options
            float        fc    = 0.20;  // normalized cutoff frequency
            // generate time vector, centered at zero
            float t = (float)index + 0.5f - 0.5f*(float)filter_len;
            // generate sinc function (time offset in 't' prevents divide by zero)
            float s = sinf(2*M_PI*fc*t + 1e-6f) / (2*M_PI*fc*t + 1e-6f);
            // generate Hamming window
            float w = 0.53836 - 0.46164*cosf((2*M_PI*(float)index)/((float)(filter_len-1)));
            // generate composite filter coefficient
            return (s * w)*(1<<14);
	}
        template <std::size_t... I>
        constexpr static std::array<short int, sizeof...(I)> coeff_fill(std::index_sequence<I...>)
        {
            return std::array<short int, sizeof...(I)>{compute_coeff(I)...};
        }
        template <std::size_t N>
        constexpr static std::array<short int, N> coeff_fill()
        {
            return coeff_fill(std::make_index_sequence<N>{});
        }

        /// filter coefficients
        static const ::std::array<short int,filter_len> coeffs;

        // array to hold input samples
        int16_t insamp[ filter_len ];



    public:

        short int operator()(short int input)
        {
            int16_t output;
            int32_t acc;     // accumulator for MACs
            int n;
            int k;

            // put the new samples at the high end of the buffer
            insamp[filter_len - 1] = input;
            // apply the filter to each input sample

            // calculate output
            // load rounding constant
            acc = (1 << 14)+((int32_t)(coeffs[0]) * (int32_t)(input));
            // perform the multiply-accumulate
            for ( k = 0; k < filter_len-1; k++ ) {
                acc += (int32_t)(coeffs[filter_len - 1 -k]) * (int32_t)(insamp[k]);
                // shift input samples back in time for next time
                insamp[k] = insamp[1+k];
            }
            // saturate the result
            if ( acc > 0x3fffffff ) {
                acc = 0x3fffffff;
            } else if ( acc < -0x40000000 ) {
                acc = -0x40000000;
            }
            // convert from Q30 to Q15
            output = (int16_t)(acc >> 15);

            return output;
        }
        firFixedClass() {}
};

template <int filter_len>
constexpr const ::std::array<short int,filter_len> firFixedClass<filter_len>::coeffs = coeff_fill<filter_len>();


// maximum length of filter than can be handled
#define FILTER_LEN     63


// the FIR filter function
#ifdef WITH_EXTERNC
extern "C" 
#endif
short int 
__attribute__ ((noinline))  
firFixed( short int input)
{
    static firFixedClass<FILTER_LEN> fir;
    return fir(input);
}

//////////////////////////////////////////////////////////////
//  Test program
//////////////////////////////////////////////////////////////
#ifndef __builtin_bambu_time_start
extern "C" void __builtin_bambu_time_start();
#endif
#ifndef __builtin_bambu_time_stop
extern "C" void __builtin_bambu_time_stop();
#endif

int main( void )
{
    int size;
    int16_t input;
    int16_t output;
    int  in_fid;
    int   out_fid;

    // open the input waveform file
    in_fid = open( "input.pcm", O_RDONLY );
    if ( in_fid < 0 ) {
#ifndef NDEBUG
        printf("couldn't open input.pcm");
#endif
        return 1;
    }

    // open the output waveform file
    out_fid = open( "outputFixed.pcm", O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP|S_IROTH|S_IWOTH );
    if ( out_fid < 0 ) {
#ifndef NDEBUG
        printf("couldn't open outputFixed.pcm");
#endif
        return 1;
    }

    // process all of the samples
    do {
        // read samples from file
        size = read(in_fid, &input, sizeof(int16_t))/sizeof(int16_t);
        // perform the filtering
        if(size)
        {
            assert(size==1);
            __builtin_bambu_time_start();
            output = firFixed( input );
            __builtin_bambu_time_stop();
            // write samples to file
            write(out_fid, &output, sizeof(int16_t));
        }
    } while ( size != 0 );

    close( in_fid );
    close( out_fid );

    return 0;
}
