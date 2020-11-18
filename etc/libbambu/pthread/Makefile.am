#this file is libbambu/pthread/Makefile.am
METASOURCES = AUTO
CC=$(top_builddir)/src/tree-panda-gcc
CFLAGS=
pkglib_LIBRARIES =
common_treegccflags = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/gcc_plugin -finline-limit=10000 -O3 -I$(BOOST_DIR)
common_treegccflags_clang = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/clang_plugin -O3 -I$(BOOST_DIR) --extra-gcc-options="-mllvm -inline-threshold=10000"

if BUILD_I386_GCC45
pkglib_LIBRARIES += libpthread_gcc45.a
libpthread_gcc45_a_SOURCES = pthread.c
libpthread_gcc45_a_CFLAGS = --compiler=I386_GCC45 $(common_treegccflags)
endif

if BUILD_I386_GCC46
pkglib_LIBRARIES += libpthread_gcc46.a
libpthread_gcc46_a_SOURCES = pthread.c
libpthread_gcc46_a_CFLAGS = --compiler=I386_GCC46  $(common_treegccflags)
endif

if BUILD_I386_GCC47
pkglib_LIBRARIES += libpthread_gcc47.a
libpthread_gcc47_a_SOURCES = pthread.c
libpthread_gcc47_a_CFLAGS = --compiler=I386_GCC47  $(common_treegccflags)
endif

if BUILD_I386_GCC48
pkglib_LIBRARIES += libpthread_gcc48.a
libpthread_gcc48_a_SOURCES = pthread.c
libpthread_gcc48_a_CFLAGS = -fno-tree-loop-distribute-patterns --compiler=I386_GCC48 $(common_treegccflags)
endif

if BUILD_I386_GCC49
pkglib_LIBRARIES += libpthread_gcc49.a
libpthread_gcc49_a_SOURCES = pthread.c
libpthread_gcc49_a_CFLAGS = -fno-tree-loop-distribute-patterns --compiler=I386_GCC49 $(common_treegccflags)
endif

if BUILD_I386_GCC5
pkglib_LIBRARIES += libpthread_gcc5.a
libpthread_gcc5_a_SOURCES = pthread.c
libpthread_gcc5_a_CFLAGS = -fno-tree-loop-distribute-patterns --compiler=I386_GCC5 $(common_treegccflags)
endif

if BUILD_I386_GCC6
pkglib_LIBRARIES += libpthread_gcc6.a
libpthread_gcc6_a_SOURCES = pthread.c
libpthread_gcc6_a_CFLAGS = -fno-tree-loop-distribute-patterns --compiler=I386_GCC6 $(common_treegccflags)
endif

if BUILD_I386_GCC7
pkglib_LIBRARIES += libpthread_gcc7.a
libpthread_gcc7_a_SOURCES = pthread.c
libpthread_gcc7_a_CFLAGS = -fno-tree-loop-distribute-patterns --compiler=I386_GCC7 $(common_treegccflags)
endif

if BUILD_I386_GCC8
pkglib_LIBRARIES += libpthread_gcc8.a
libpthread_gcc8_a_SOURCES = pthread.c
libpthread_gcc8_a_CFLAGS = -fno-tree-loop-distribute-patterns --compiler=I386_GCC8 $(common_treegccflags)
endif

if BUILD_I386_CLANG4
pkglib_LIBRARIES += libpthread_clang4.a
libpthread_clang4_a_SOURCES = pthread.c
libpthread_clang4_a_CFLAGS = --compiler=I386_CLANG4 $(common_treegccflags_clang)
endif

if BUILD_I386_CLANG5
pkglib_LIBRARIES += libpthread_clang5.a
libpthread_clang5_a_SOURCES = pthread.c
libpthread_clang5_a_CFLAGS = --compiler=I386_CLANG5 $(common_treegccflags_clang)
endif

if BUILD_I386_CLANG6
pkglib_LIBRARIES += libpthread_clang6.a
libpthread_clang6_a_SOURCES = pthread.c
libpthread_clang6_a_CFLAGS = --compiler=I386_CLANG6 $(common_treegccflags_clang)
endif

if BUILD_I386_CLANG7
pkglib_LIBRARIES += libpthread_clang7.a
libpthread_clang7_a_SOURCES = pthread.c
libpthread_clang7_a_CFLAGS = --compiler=I386_CLANG7 $(common_treegccflags_clang)
endif

if BUILD_I386_CLANG8
pkglib_LIBRARIES += libpthread_clang8.a
libpthread_clang8_a_SOURCES = pthread.c
libpthread_clang8_a_CFLAGS = --compiler=I386_CLANG8 $(common_treegccflags_clang)
endif

if BUILD_I386_CLANG9
pkglib_LIBRARIES += libpthread_clang9.a
libpthread_clang9_a_SOURCES = pthread.c
libpthread_clang9_a_CFLAGS = --compiler=I386_CLANG9 $(common_treegccflags_clang)
endif

if BUILD_I386_CLANG10
pkglib_LIBRARIES += libpthread_clang10.a
libpthread_clang10_a_SOURCES = pthread.c
libpthread_clang10_a_CFLAGS = --compiler=I386_CLANG10 $(common_treegccflags_clang)
endif

if BUILD_I386_CLANG11
pkglib_LIBRARIES += libpthread_clang11.a
libpthread_clang11_a_SOURCES = pthread.c
libpthread_clang11_a_CFLAGS = --compiler=I386_CLANG11 $(common_treegccflags_clang)
endif

