This is a minimal port of ac\_math (https://github.com/hlslibs/ac_math) to bambu/PANDA. The changes are mainly related to fixes of warnings that clang has and GCC does not. The c++14 standard used to compile the regression tests is mainly required by the bambu/PANDA ac_types porting.
All the tests pass with both GCC and Clang. Clang works only with the ac_types port of bambu/PANDA.
The time to compile and run the tests are quite similar to the one obtained with the original libraries. 
They require:
 - 164.46user 3.99system 2:48.58elapsed 99%CPU with GCC 7.3.0 and the original ac_types library;
 - 153.94user 2.61system 2:37.23elapsed 99%CPU with CLANG 6 and the bambu/PANDA ac_types porting;
 - 181.84user 4.46system 3:06.46elapsed 99%CPU with GCC 7.3.0 and the bambu/PANDA ac_types porting.
