#this file is libbambu/libm/Makefile.am
EXTRA_DIST=math_constants.h math_privatef.h math_private.h math_privatetf.h ef_rem_pio2.c sf_sin.c sf_cos.c wf_sincos.c sf_tan.c kf_rem_pio2.c erf_lgamma.c math_private_kernels.c s_sin.c s_cos.c s_tan.c w_sincos.c COPYING.LIB COPYING.LIBM_PANDA README_PANDA.txt

METASOURCES = AUTO
CC=$(top_builddir)/src/tree-panda-gcc
CFLAGS=
AM_CFLAGS=
pkglib_LIBRARIES =
common_libm_src = \
   ef_acos.c ef_acosh.c ef_asin.c ef_atan2.c ef_atanh.c ef_cosh.c ef_fmod.c ef_hypot.c ef_j0.c ef_j1.c ef_jn.c \
   ef_log10.c ef_remainder.c ef_scalb.c ef_sinh.c \
   ew_acos.c ew_acosh.c ew_asin.c ew_atan2.c ew_atanh.c ew_cosh.c ew_fmod.c ew_gamma.c ew_hypot.c ew_j0.c \
   ew_log10.c ew_pow.c ew_remainder.c ew_sinh.c \
   kf_cos.c kf_sin.c kf_tan.c \
   s_asinh.c s_atan.c s_cbrt.c s_ceil.c s_common.c s_erf.c s_expm1.c s_floor.c s_fmax.c s_fmin.c s_frexp.c \
   s_ilogb.c s_lceil.c s_ldexp.c s_lfloor.c s_llrint.c s_log1p.c s_logb.c s_lrint.c s_modf.c s_nearbyint.c \
   s_nextafter.c s_rint.c s_round.c s_scalbn.c s_significand.c s_tanh.c s_trigon.c \
   sf_asinh.c sf_atan.c sf_cbrt.c sf_ceil.c sf_erf.c sf_exp10.c sf_expm1.c sf_fdim.c sf_floor.c sf_fma.c sf_fmax.c \
   sf_fmin.c sf_frexp.c sf_ilogb.c sf_lceil.c sf_ldexp.c sf_lfloor.c sf_llrint.c sf_llround.c sf_log1p.c sf_log2.c \
   sf_logb.c sf_lrint.c sf_lround.c sf_modf.c sf_nearbyint.c sf_nextafter.c sf_pow10.c sf_remquo.c sf_rint.c \
   sf_round.c sf_scalbln.c sf_scalbn.c sf_signif.c sf_tanh.c sf_trunc.c \
   tf_copysign.c tf_fabs.c tf_fpclassify.c tf_inf.c tf_isfinite.c tf_isinf.c tf_isinf_sign.c tf_isnan.c tf_isnormal.c \
   tf_nan.c tf_nans.c tf_signbit.c \
   w_exp.c w_exp2.c w_log.c w_scalb.c \
   wf_acos.c wf_acosh.c wf_asin.c wf_atan2.c wf_atanh.c wf_cosh.c wf_drem.c wf_exp2.c wf_fmod.c wf_gamma.c wf_hypot.c \
   wf_j0.c wf_j1.c wf_jn.c wf_lgamma.c wf_log10.c wf_remainder.c wf_scalb.c wf_sinh.c wf_tgamma.c \
   wrf_gamma.c wrf_lgamma.c

common_libm_src_fr=hotbm_expf.c hotbm_logf.c hotbm_sine_cosine.c poly_sqrt.c poly_sqrtf.c wf_pow_opt.c
common_libm_src_std=ef_exp.c ef_log.c ef_pow.c ef_sqrt.c ew_sqrt.c sf_trigon.c wf_exp.c wf_log.c wf_pow.c wf_sqrt.c

if BUILD_FLOPOCO
common_libm_src += s_copysign.c sf_copysign.c
endif
common_treegccflags = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/gcc_plugin
common_treegccflags_clang = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/clang_plugin
common_cflags=-DNO_RAISE_EXCEPTIONS -I$(top_srcdir)/etc/libbambu $(common_treegccflags) -I$(BOOST_DIR)
common_cflags_clang=-DNO_RAISE_EXCEPTIONS -I$(top_srcdir)/etc/libbambu $(common_treegccflags_clang) -I$(BOOST_DIR)
compiler_flags=-O2 -fipa-pta -fgcse-after-reload -fipa-cp-clone -funswitch-loops -finline-functions -fno-tree-loop-ivcanon $(common_cflags)
compiler_flags_clang=-O2 -finline-functions -finline-hint-functions $(common_cflags_clang)
compiler_flags_nosub=-DFLT_NO_DENORMALS -DNO_SUBNORMALS $(compiler_flags)
compiler_flags_clang_nosub=-DFLT_NO_DENORMALS -DNO_SUBNORMALS $(compiler_flags_clang)

if BUILD_I386_GCC49
pkglib_LIBRARIES += libm_gcc49.a libm_gcc49_std.a
libm_gcc49_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc49_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC49
libm_gcc49_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C
libm_gcc49_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc49_std_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC49
libm_gcc49_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C

pkglib_LIBRARIES += libm_gcc49_subnormals.a libm_gcc49_subnormals_std.a
libm_gcc49_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc49_subnormals_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC49
libm_gcc49_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C
libm_gcc49_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc49_subnormals_std_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC49
libm_gcc49_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C
endif

if BUILD_I386_GCC5
pkglib_LIBRARIES += libm_gcc5.a libm_gcc5_std.a
libm_gcc5_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc5_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC5
libm_gcc5_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C
libm_gcc5_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc5_std_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC5
libm_gcc5_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C

pkglib_LIBRARIES += libm_gcc5_subnormals.a libm_gcc5_subnormals_std.a
libm_gcc5_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc5_subnormals_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC5
libm_gcc5_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C
libm_gcc5_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc5_subnormals_std_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC5
libm_gcc5_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C
endif

if BUILD_I386_GCC6
pkglib_LIBRARIES += libm_gcc6.a libm_gcc6_std.a
libm_gcc6_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc6_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC6
libm_gcc6_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C
libm_gcc6_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc6_std_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC6
libm_gcc6_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C

pkglib_LIBRARIES += libm_gcc6_subnormals.a libm_gcc6_subnormals_std.a
libm_gcc6_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc6_subnormals_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC6
libm_gcc6_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C
libm_gcc6_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc6_subnormals_std_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC6
libm_gcc6_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C
endif

if BUILD_I386_GCC7
pkglib_LIBRARIES += libm_gcc7.a libm_gcc7_std.a
libm_gcc7_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc7_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC7
libm_gcc7_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
libm_gcc7_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc7_std_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC7
libm_gcc7_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C

pkglib_LIBRARIES += libm_gcc7_subnormals.a libm_gcc7_subnormals_std.a
libm_gcc7_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc7_subnormals_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC7
libm_gcc7_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
libm_gcc7_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc7_subnormals_std_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC7
libm_gcc7_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
endif

if BUILD_I386_GCC8
pkglib_LIBRARIES += libm_gcc8.a libm_gcc8_std.a
libm_gcc8_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc8_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC8
libm_gcc8_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC8 $(common_treegccflags) -C
libm_gcc8_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc8_std_a_CFLAGS = $(compiler_flags_nosub) -ftree-partial-pre --compiler=I386_GCC8
libm_gcc8_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC8 $(common_treegccflags) -C

pkglib_LIBRARIES += libm_gcc8_subnormals.a libm_gcc8_subnormals_std.a
libm_gcc8_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_gcc8_subnormals_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC8
libm_gcc8_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC8 $(common_treegccflags) -C
libm_gcc8_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_gcc8_subnormals_std_a_CFLAGS = $(compiler_flags) -ftree-partial-pre --compiler=I386_GCC8
libm_gcc8_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC8 $(common_treegccflags) -C
endif


if BUILD_I386_CLANG4
pkglib_LIBRARIES += libm_clang4.a libm_clang4_std.a
libm_clang4_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang4_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG4
libm_clang4_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG4 $(common_treegccflags_clang) -C
libm_clang4_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang4_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG4
libm_clang4_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG4 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang4_subnormals.a libm_clang4_subnormals_std.a
libm_clang4_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang4_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG4
libm_clang4_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG4 $(common_treegccflags_clang) -C
libm_clang4_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang4_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG4
libm_clang4_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG4 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG5
pkglib_LIBRARIES += libm_clang5.a libm_clang5_std.a
libm_clang5_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang5_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG5
libm_clang5_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG5 $(common_treegccflags_clang) -C
libm_clang5_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang5_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG5
libm_clang5_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG5 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang5_subnormals.a libm_clang5_subnormals_std.a
libm_clang5_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang5_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG5
libm_clang5_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG5 $(common_treegccflags_clang) -C
libm_clang5_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang5_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG5
libm_clang5_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG5 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG6
pkglib_LIBRARIES += libm_clang6.a libm_clang6_std.a
libm_clang6_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang6_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG6
libm_clang6_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG6 $(common_treegccflags_clang) -C
libm_clang6_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang6_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG6
libm_clang6_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG6 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang6_subnormals.a libm_clang6_subnormals_std.a
libm_clang6_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang6_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG6
libm_clang6_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG6 $(common_treegccflags_clang) -C
libm_clang6_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang6_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG6
libm_clang6_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG6 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG7
pkglib_LIBRARIES += libm_clang7.a libm_clang7_std.a
libm_clang7_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang7_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG7
libm_clang7_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG7 $(common_treegccflags_clang) -C
libm_clang7_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang7_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG7
libm_clang7_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG7 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang7_subnormals.a libm_clang7_subnormals_std.a
libm_clang7_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang7_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG7
libm_clang7_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG7 $(common_treegccflags_clang) -C
libm_clang7_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang7_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG7
libm_clang7_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG7 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG8
pkglib_LIBRARIES += libm_clang8.a libm_clang8_std.a
libm_clang8_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang8_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG8
libm_clang8_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG8 $(common_treegccflags_clang) -C
libm_clang8_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang8_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG8
libm_clang8_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG8 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang8_subnormals.a libm_clang8_subnormals_std.a
libm_clang8_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang8_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG8
libm_clang8_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG8 $(common_treegccflags_clang) -C
libm_clang8_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang8_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG8
libm_clang8_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG8 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG9
pkglib_LIBRARIES += libm_clang9.a libm_clang9_std.a
libm_clang9_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang9_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG9
libm_clang9_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG9 $(common_treegccflags_clang) -C
libm_clang9_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang9_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG9
libm_clang9_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG9 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang9_subnormals.a libm_clang9_subnormals_std.a
libm_clang9_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang9_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG9
libm_clang9_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG9 $(common_treegccflags_clang) -C
libm_clang9_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang9_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG9
libm_clang9_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG9 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG10
pkglib_LIBRARIES += libm_clang10.a libm_clang10_std.a
libm_clang10_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang10_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG10
libm_clang10_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG10 $(common_treegccflags_clang) -C
libm_clang10_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang10_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG10
libm_clang10_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG10 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang10_subnormals.a libm_clang10_subnormals_std.a
libm_clang10_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang10_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG10
libm_clang10_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG10 $(common_treegccflags_clang) -C
libm_clang10_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang10_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG10
libm_clang10_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG10 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG11
pkglib_LIBRARIES += libm_clang11.a libm_clang11_std.a
libm_clang11_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang11_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG11
libm_clang11_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG11 $(common_treegccflags_clang) -C
libm_clang11_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang11_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG11
libm_clang11_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG11 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang11_subnormals.a libm_clang11_subnormals_std.a
libm_clang11_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang11_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG11
libm_clang11_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG11 $(common_treegccflags_clang) -C
libm_clang11_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang11_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG11
libm_clang11_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG11 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANGVVD
pkglib_LIBRARIES += libm_clangvvd.a libm_clangvvd_std.a
libm_clangvvd_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clangvvd_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANGVVD
libm_clangvvd_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANGVVD $(common_treegccflags_clang) -C
libm_clangvvd_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clangvvd_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANGVVD
libm_clangvvd_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANGVVD $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clangvvd_subnormals.a libm_clangvvd_subnormals_std.a
libm_clangvvd_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clangvvd_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANGVVD
libm_clangvvd_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANGVVD $(common_treegccflags_clang) -C
libm_clangvvd_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clangvvd_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANGVVD
libm_clangvvd_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANGVVD $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG12
pkglib_LIBRARIES += libm_clang12.a libm_clang12_std.a
libm_clang12_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang12_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG12
libm_clang12_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG12 $(common_treegccflags_clang) -C
libm_clang12_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang12_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG12
libm_clang12_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG12 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang12_subnormals.a libm_clang12_subnormals_std.a
libm_clang12_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang12_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG12
libm_clang12_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG12 $(common_treegccflags_clang) -C
libm_clang12_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang12_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG12
libm_clang12_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG12 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG13
pkglib_LIBRARIES += libm_clang13.a libm_clang13_std.a
libm_clang13_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang13_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG13
libm_clang13_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG13 $(common_treegccflags_clang) -C
libm_clang13_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang13_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG13
libm_clang13_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG13 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang13_subnormals.a libm_clang13_subnormals_std.a
libm_clang13_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang13_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG13
libm_clang13_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG13 $(common_treegccflags_clang) -C
libm_clang13_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang13_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG13
libm_clang13_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG13 $(common_treegccflags_clang) -C
endif

if BUILD_I386_CLANG16
pkglib_LIBRARIES += libm_clang16.a libm_clang16_std.a
libm_clang16_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang16_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG16
libm_clang16_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG16 $(common_treegccflags_clang) -C
libm_clang16_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang16_std_a_CFLAGS = $(compiler_flags_clang_nosub) --compiler=I386_CLANG16
libm_clang16_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG16 $(common_treegccflags_clang) -C

pkglib_LIBRARIES += libm_clang16_subnormals.a libm_clang16_subnormals_std.a
libm_clang16_subnormals_a_SOURCES = $(common_libm_src) $(common_libm_src_fr)
libm_clang16_subnormals_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG16
libm_clang16_subnormals_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG16 $(common_treegccflags_clang) -C
libm_clang16_subnormals_std_a_SOURCES = $(common_libm_src) $(common_libm_src_std)
libm_clang16_subnormals_std_a_CFLAGS = $(compiler_flags_clang) --compiler=I386_CLANG16
libm_clang16_subnormals_std_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_CLANG16 $(common_treegccflags_clang) -C
endif

DIST_SOURCES = $(common_libm_src) $(common_libm_src_fr) $(common_libm_src_std)
