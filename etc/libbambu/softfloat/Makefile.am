#this file is libbambu/softfloat/Makefile.am
EXTRA_DIST=bambu.h milieu.h README.txt softfloat.h SoftFloat-history.txt softfloat-macros SoftFloat-source.txt softfloat-specialize SoftFloat.txt timesoftfloat.c timesoftfloat.txt README_PANDA.txt div_utilities

common_treegccflags = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/gcc_plugin
common_cflags=-DNO_PARAMETRIC -DNO_SUBNORMALS -DNO_SIGNALLING -I$(top_srcdir)/libbambu $(common_treegccflags)
common_cflags_subnormals=-DNO_PARAMETRIC -DNO_SIGNALLING -I$(top_srcdir)/libbambu $(common_treegccflags)

METASOURCES = AUTO
CC=../$(top_builddir)/src/tree-panda-gcc
CFLAGS=
pkglib_LIBRARIES = 

if BUILD_I386_GCC45
pkglib_LIBRARIES += libsoftfloat_gcc45.a libsoftfloat_subnormals_gcc45.a
libsoftfloat_gcc45_a_SOURCES = softfloat.c
libsoftfloat_gcc45_a_CFLAGS = -O3 --compiler=I386_GCC45 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc45_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc45_a_CFLAGS = -O3 --compiler=I386_GCC45 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC46
pkglib_LIBRARIES += libsoftfloat_gcc46.a libsoftfloat_subnormals_gcc46.a
libsoftfloat_gcc46_a_SOURCES = softfloat.c
libsoftfloat_gcc46_a_CFLAGS = -O3 --compiler=I386_GCC46 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc46_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc46_a_CFLAGS = -O3 --compiler=I386_GCC46 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC47
pkglib_LIBRARIES += libsoftfloat_gcc47.a libsoftfloat_subnormals_gcc47.a
libsoftfloat_gcc47_a_SOURCES = softfloat.c
libsoftfloat_gcc47_a_CFLAGS = -O3 --compiler=I386_GCC47 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc47_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc47_a_CFLAGS = -O3 --compiler=I386_GCC47 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC48
pkglib_LIBRARIES += libsoftfloat_gcc48.a libsoftfloat_subnormals_gcc48.a
libsoftfloat_gcc48_a_SOURCES = softfloat.c
libsoftfloat_gcc48_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC48 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc48_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc48_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC48 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC49
pkglib_LIBRARIES += libsoftfloat_gcc49.a libsoftfloat_subnormals_gcc49.a
libsoftfloat_gcc49_a_SOURCES = softfloat.c
libsoftfloat_gcc49_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC49 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc49_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc49_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC49 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC5
pkglib_LIBRARIES += libsoftfloat_gcc5.a libsoftfloat_subnormals_gcc5.a
libsoftfloat_gcc5_a_SOURCES = softfloat.c
libsoftfloat_gcc5_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC5 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc5_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc5_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC5 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC6
pkglib_LIBRARIES += libsoftfloat_gcc6.a libsoftfloat_subnormals_gcc6.a
libsoftfloat_gcc6_a_SOURCES = softfloat.c
libsoftfloat_gcc6_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC6 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc6_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc6_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC6 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
endif

if BUILD_I386_GCC7
pkglib_LIBRARIES += libsoftfloat_gcc7.a libsoftfloat_subnormals_gcc7.a
libsoftfloat_gcc7_a_SOURCES = softfloat.c
libsoftfloat_gcc7_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC7 -finline-limit=10000 $(common_cflags) -I$(BOOST_DIR)
libsoftfloat_subnormals_gcc7_a_SOURCES = softfloat.c
libsoftfloat_subnormals_gcc7_a_CFLAGS = -O3 -fno-tree-loop-distribute-patterns --compiler=I386_GCC7 -finline-limit=10000 $(common_cflags_subnormals) -I$(BOOST_DIR)
endif

#do not touch the following line
 
