#this file is libbambu/softfloat/Makefile.am
EXTRA_DIST=bambu.h milieu.h README.txt softfloat.h SoftFloat-history.txt softfloat-macros SoftFloat-source.txt softfloat-specialize SoftFloat.txt timesoftfloat.c timesoftfloat.txt README_PANDA.txt div_utilities

common_treegccflags = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/gcc_plugin
common_treegccflags_clang = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/clang_plugin
common_cflags=-DNO_PARAMETRIC -DNO_SUBNORMALS -DNO_SIGNALLING -I$(top_srcdir)/etc/libbambu $(common_treegccflags)
common_cflags_clang=-DNO_PARAMETRIC -DNO_SUBNORMALS -DNO_SIGNALLING -I$(top_srcdir)/etc/libbambu $(common_treegccflags_clang) 
common_cflags_subnormals=-DNO_PARAMETRIC -DNO_SIGNALLING -I$(top_srcdir)/etc/libbambu $(common_treegccflags)
common_cflags_subnormals_clang=-DNO_PARAMETRIC -DNO_SIGNALLING -I$(top_srcdir)/etc/libbambu $(common_treegccflags_clang)
common_cflags_norounding=-DNO_PARAMETRIC -DNO_SUBNORMALS -DNO_SIGNALLING -DNO_ROUNDING -I$(top_srcdir)/etc/libbambu $(common_treegccflags)
common_cflags_norounding_clang=-DNO_PARAMETRIC -DNO_SUBNORMALS -DNO_SIGNALLING -DNOROUNDING -I$(top_srcdir)/etc/libbambu $(common_treegccflags_clang)

METASOURCES = AUTO
CC=$(top_builddir)/src/tree-panda-gcc
CFLAGS=
pkglib_LIBRARIES = 

if BUILD_I386_GCC45
pkglib_LIBRARIES += libsoftfloat_gcc45.a libsoftfloat_subnormals_gcc45.a libsoftfloat_norounding_gcc45.a
libsoftfloat_gcc45_a_SOURCES = softfloat.c
libsoftfloat_gcc45_a_CFLAGS = -O3 --compiler=I386_GCC45 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc45_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc45_a_CFLAGS = -O3 --compiler=I386_GCC45 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc45_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc45_a_CFLAGS = -O3 --compiler=I386_GCC45 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC46
pkglib_LIBRARIES += libsoftfloat_gcc46.a libsoftfloat_subnormals_gcc46.a libsoftfloat_norounding_gcc46.a
libsoftfloat_gcc46_a_SOURCES = softfloat.c
libsoftfloat_gcc46_a_CFLAGS = -O3 --compiler=I386_GCC46 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc46_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc46_a_CFLAGS = -O3 --compiler=I386_GCC46 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc46_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc46_a_CFLAGS = -O3 --compiler=I386_GCC46 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC47
pkglib_LIBRARIES += libsoftfloat_gcc47.a libsoftfloat_subnormals_gcc47.a libsoftfloat_norounding_gcc47.a
libsoftfloat_gcc47_a_SOURCES = softfloat.c
libsoftfloat_gcc47_a_CFLAGS = -O3 --compiler=I386_GCC47 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc47_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc47_a_CFLAGS = -O3 --compiler=I386_GCC47 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc47_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc47_a_CFLAGS = -O3 --compiler=I386_GCC47 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC48
pkglib_LIBRARIES += libsoftfloat_gcc48.a libsoftfloat_subnormals_gcc48.a libsoftfloat_norounding_gcc48.a
libsoftfloat_gcc48_a_SOURCES = softfloat.c
libsoftfloat_gcc48_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC48 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc48_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc48_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC48 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc48_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc48_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC48 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC49
pkglib_LIBRARIES += libsoftfloat_gcc49.a libsoftfloat_subnormals_gcc49.a libsoftfloat_norounding_gcc49.a
libsoftfloat_gcc49_a_SOURCES = softfloat.c
libsoftfloat_gcc49_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC49 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc49_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc49_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC49 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc49_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc49_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC49 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC5
pkglib_LIBRARIES += libsoftfloat_gcc5.a libsoftfloat_subnormals_gcc5.a libsoftfloat_norounding_gcc5.a
libsoftfloat_gcc5_a_SOURCES = softfloat.c
libsoftfloat_gcc5_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC5 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc5_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc5_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC5 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc5_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc5_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC5 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC6
pkglib_LIBRARIES += libsoftfloat_gcc6.a libsoftfloat_subnormals_gcc6.a libsoftfloat_norounding_gcc6.a
libsoftfloat_gcc6_a_SOURCES = softfloat.c
libsoftfloat_gcc6_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC6 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc6_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc6_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC6 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc6_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc6_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC6 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC7
pkglib_LIBRARIES += libsoftfloat_gcc7.a libsoftfloat_subnormals_gcc7.a libsoftfloat_norounding_gcc7.a
libsoftfloat_gcc7_a_SOURCES = softfloat.c
libsoftfloat_gcc7_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC7 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc7_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc7_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC7 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc7_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc7_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC7 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC8
pkglib_LIBRARIES += libsoftfloat_gcc8.a libsoftfloat_subnormals_gcc8.a libsoftfloat_norounding_gcc8.a
libsoftfloat_gcc8_a_SOURCES = softfloat.c
libsoftfloat_gcc8_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC8 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc8_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc8_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC8 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
libsoftfloat_norounding_gcc8_a_SOURCES = softfloat.c
libsoftfloat_norounding_gcc8_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC8 -finline-limit=10000 $(common_cflags_norounding) -I$(BOOST_DIR)
endif

if BUILD_I386_CLANG4
pkglib_LIBRARIES += libsoftfloat_clang4.a libsoftfloat_subnormals_clang4.a libsoftfloat_norounding_clang4.a
libsoftfloat_clang4_a_SOURCES = softfloat.c
libsoftfloat_clang4_a_CFLAGS = -O3 --compiler=I386_CLANG4 $(common_cflags_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_subnormals_clang4_a_SOURCES = softfloat.c
libsoftfloat_subnormals_clang4_a_CFLAGS = -O3 --compiler=I386_CLANG4 $(common_cflags_subnormals_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_norounding_clang4_a_SOURCES = softfloat.c
libsoftfloat_norounding_clang4_a_CFLAGS = -O3 --compiler=I386_CLANG4 $(common_cflags_norounding_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
endif 

if BUILD_I386_CLANG5
pkglib_LIBRARIES += libsoftfloat_clang5.a libsoftfloat_subnormals_clang5.a libsoftfloat_norounding_clang5.a
libsoftfloat_clang5_a_SOURCES = softfloat.c
libsoftfloat_clang5_a_CFLAGS = -O3 --compiler=I386_CLANG5 $(common_cflags_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_subnormals_clang5_a_SOURCES = softfloat.c
libsoftfloat_subnormals_clang5_a_CFLAGS = -O3 --compiler=I386_CLANG5 $(common_cflags_subnormals_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_norounding_clang5_a_SOURCES = softfloat.c
libsoftfloat_norounding_clang5_a_CFLAGS = -O3 --compiler=I386_CLANG5 $(common_cflags_norounding_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
endif 

if BUILD_I386_CLANG6
pkglib_LIBRARIES += libsoftfloat_clang6.a libsoftfloat_subnormals_clang6.a libsoftfloat_norounding_clang6.a
libsoftfloat_clang6_a_SOURCES = softfloat.c
libsoftfloat_clang6_a_CFLAGS = -O3 --compiler=I386_CLANG6 $(common_cflags_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_subnormals_clang6_a_SOURCES = softfloat.c
libsoftfloat_subnormals_clang6_a_CFLAGS = -O3 --compiler=I386_CLANG6 $(common_cflags_subnormals_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_norounding_clang6_a_SOURCES = softfloat.c
libsoftfloat_norounding_clang6_a_CFLAGS = -O3 --compiler=I386_CLANG6 $(common_cflags_norounding_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
endif 

if BUILD_I386_CLANG7
pkglib_LIBRARIES += libsoftfloat_clang7.a libsoftfloat_subnormals_clang7.a libsoftfloat_norounding_clang7.a
libsoftfloat_clang7_a_SOURCES = softfloat.c
libsoftfloat_clang7_a_CFLAGS = -O3 --compiler=I386_CLANG7 $(common_cflags_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_subnormals_clang7_a_SOURCES = softfloat.c
libsoftfloat_subnormals_clang7_a_CFLAGS = -O3 --compiler=I386_CLANG7 $(common_cflags_subnormals_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_norounding_clang7_a_SOURCES = softfloat.c
libsoftfloat_norounding_clang7_a_CFLAGS = -O3 --compiler=I386_CLANG7 $(common_cflags_norounding_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
endif 

if BUILD_I386_CLANG8
pkglib_LIBRARIES += libsoftfloat_clang8.a libsoftfloat_subnormals_clang8.a libsoftfloat_norounding_clang8.a
libsoftfloat_clang8_a_SOURCES = softfloat.c
libsoftfloat_clang8_a_CFLAGS = -O3 --compiler=I386_CLANG8 $(common_cflags_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_subnormals_clang8_a_SOURCES = softfloat.c
libsoftfloat_subnormals_clang8_a_CFLAGS = -O3 --compiler=I386_CLANG8 $(common_cflags_subnormals_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_norounding_clang8_a_SOURCES = softfloat.c
libsoftfloat_norounding_clang8_a_CFLAGS = -O3 --compiler=I386_CLANG8 $(common_cflags_norounding_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
endif 

if BUILD_I386_CLANG9
pkglib_LIBRARIES += libsoftfloat_clang9.a libsoftfloat_subnormals_clang9.a libsoftfloat_norounding_clang9.a
libsoftfloat_clang9_a_SOURCES = softfloat.c
libsoftfloat_clang9_a_CFLAGS = -O3 --compiler=I386_CLANG9 $(common_cflags_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_subnormals_clang9_a_SOURCES = softfloat.c
libsoftfloat_subnormals_clang9_a_CFLAGS = -O3 --compiler=I386_CLANG9 $(common_cflags_subnormals_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_norounding_clang9_a_SOURCES = softfloat.c
libsoftfloat_norounding_clang9_a_CFLAGS = -O3 --compiler=I386_CLANG9 $(common_cflags_norounding_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
endif

if BUILD_I386_CLANG10
pkglib_LIBRARIES += libsoftfloat_clang10.a libsoftfloat_subnormals_clang10.a
libsoftfloat_clang10_a_SOURCES = softfloat.c
libsoftfloat_clang10_a_CFLAGS = -O3 --compiler=I386_CLANG10 $(common_cflags_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_subnormals_clang10_a_SOURCES = softfloat.c
libsoftfloat_subnormals_clang10_a_CFLAGS = -O3 --compiler=I386_CLANG10 $(common_cflags_subnormals_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
endif

if BUILD_I386_CLANG11
pkglib_LIBRARIES += libsoftfloat_clang11.a libsoftfloat_subnormals_clang11.a
libsoftfloat_clang11_a_SOURCES = softfloat.c
libsoftfloat_clang11_a_CFLAGS = -O3 --compiler=I386_CLANG11 $(common_cflags_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
libsoftfloat_subnormals_clang11_a_SOURCES = softfloat.c
libsoftfloat_subnormals_clang11_a_CFLAGS = -O3 --compiler=I386_CLANG11 $(common_cflags_subnormals_clang) -I$(BOOST_DIR) -finline-functions --extra-gcc-options="-mllvm -inline-threshold=10000"
endif

