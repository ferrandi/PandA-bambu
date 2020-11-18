#this file is libbambu/Makefile.am
SUBDIRS = libm softfloat soft-fp pthread ac_types ac_math
DIST_SUBDIRS = libm softfloat soft-fp pthread ac_types ac_math
EXTRA_DIST= COPYING.LIB README_PANDA.txt bambu_macros.h
common_libbambu_src = memcmp.c memcpy.c memmove.c memset.c memmgr.c puts.c byteswap.c read.c write.c assert.c\
                      mul64.c \
                      string/stpcpy.c string/stpncpy.c\
                      string/strcasecmp.c string/strcasestr.c string/strcat.c string/strchr.c string/strchrnul.c\
                      string/strcmp.c string/strcpy.c string/strcspn.c string/strdup.c string/strlen.c string/strncasecmp.c\
                      string/strncat.c string/strncmp.c string/strncpy.c string/strndup.c string/strnlen.c string/strpbrk.c\
                      string/strrchr.c string/strsep.c string/strspn.c string/strstr.c string/strtok.c\
                      string/bzero.c string/bcopy.c string/mempcpy.c string/memchr.c string/memrchr.c string/rawmemchr.c

METASOURCES = AUTO
CC=$(top_builddir)/src/tree-panda-gcc
CFLAGS=
pkglib_LIBRARIES =
common_treegccflags = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/gcc_plugin
common_treegccflags_clang = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/clang_plugin
common_cflags=-O3 -nostdlib  $(common_treegccflags) $(BOOST_CPPFLAGS)
common_cflags_clang=-O3 -nostdlib  $(common_treegccflags_clang) $(BOOST_CPPFLAGS)

if BUILD_I386_GCC45
pkglib_LIBRARIES += libbambu_gcc45.a
libbambu_gcc45_a_SOURCES = $(common_libbambu_src)
libbambu_gcc45_a_CFLAGS =$(common_cflags) --compiler=I386_GCC45 
libbambu_gcc45_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC45 -C
endif

if BUILD_I386_GCC46
pkglib_LIBRARIES += libbambu_gcc46.a
libbambu_gcc46_a_SOURCES = $(common_libbambu_src)
libbambu_gcc46_a_CFLAGS =$(common_cflags) --compiler=I386_GCC46 
libbambu_gcc46_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC46 -C
endif

if BUILD_I386_GCC47
pkglib_LIBRARIES += libbambu_gcc47.a
libbambu_gcc47_a_SOURCES = $(common_libbambu_src)
libbambu_gcc47_a_CFLAGS =$(common_cflags) --compiler=I386_GCC47 
libbambu_gcc47_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC47 -C
endif

if BUILD_I386_GCC48
pkglib_LIBRARIES += libbambu_gcc48.a
libbambu_gcc48_a_SOURCES = $(common_libbambu_src)
libbambu_gcc48_a_CFLAGS =$(common_cflags) -fno-tree-loop-distribute-patterns --compiler=I386_GCC48 
libbambu_gcc48_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC48 -C
endif

if BUILD_I386_GCC49
pkglib_LIBRARIES += libbambu_gcc49.a
libbambu_gcc49_a_SOURCES = $(common_libbambu_src)
libbambu_gcc49_a_CFLAGS =$(common_cflags) -fno-tree-loop-distribute-patterns --compiler=I386_GCC49 
libbambu_gcc49_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC49 -C
endif

if BUILD_I386_GCC5
pkglib_LIBRARIES += libbambu_gcc5.a
libbambu_gcc5_a_SOURCES =$(common_libbambu_src)
libbambu_gcc5_a_CFLAGS =$(common_cflags) -fno-tree-loop-distribute-patterns --compiler=I386_GCC5 
libbambu_gcc5_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC5 -C
endif

if BUILD_I386_GCC6
pkglib_LIBRARIES += libbambu_gcc6.a
libbambu_gcc6_a_SOURCES = $(common_libbambu_src)
libbambu_gcc6_a_CFLAGS =$(common_cflags) -fno-tree-loop-distribute-patterns --compiler=I386_GCC6 
libbambu_gcc6_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC6 -C
endif

if BUILD_I386_GCC7
pkglib_LIBRARIES += libbambu_gcc7.a
libbambu_gcc7_a_SOURCES = $(common_libbambu_src)
libbambu_gcc7_a_CFLAGS =$(common_cflags) -fno-tree-loop-distribute-patterns --compiler=I386_GCC7 
libbambu_gcc7_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC7 -C
endif

if BUILD_I386_GCC8
pkglib_LIBRARIES += libbambu_gcc8.a
libbambu_gcc8_a_SOURCES = $(common_libbambu_src)
libbambu_gcc8_a_CFLAGS =$(common_cflags) -fno-tree-loop-distribute-patterns --compiler=I386_GCC8 
libbambu_gcc8_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags) --compiler=I386_GCC8 -C
endif

if BUILD_I386_CLANG4
pkglib_LIBRARIES += libbambu_clang4.a
libbambu_clang4_a_SOURCES = $(common_libbambu_src)
libbambu_clang4_a_CFLAGS =$(common_cflags_clang) --compiler=I386_CLANG4 
libbambu_clang4_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags_clang) --compiler=I386_CLANG4 -C
endif

if BUILD_I386_CLANG5
pkglib_LIBRARIES += libbambu_clang5.a
libbambu_clang5_a_SOURCES = $(common_libbambu_src)
libbambu_clang5_a_CFLAGS =$(common_cflags_clang) --compiler=I386_CLANG5 
libbambu_clang5_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags_clang) --compiler=I386_CLANG5 -C
endif

if BUILD_I386_CLANG6
pkglib_LIBRARIES += libbambu_clang6.a
libbambu_clang6_a_SOURCES = $(common_libbambu_src)
libbambu_clang6_a_CFLAGS =$(common_cflags_clang) --compiler=I386_CLANG6
libbambu_clang6_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags_clang) --compiler=I386_CLANG6 -C
endif

if BUILD_I386_CLANG7
pkglib_LIBRARIES += libbambu_clang7.a
libbambu_clang7_a_SOURCES = $(common_libbambu_src)
libbambu_clang7_a_CFLAGS =$(common_cflags_clang) --compiler=I386_CLANG7
libbambu_clang7_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags_clang) --compiler=I386_CLANG7 -C
endif

if BUILD_I386_CLANG8
pkglib_LIBRARIES += libbambu_clang8.a
libbambu_clang8_a_SOURCES = $(common_libbambu_src)
libbambu_clang8_a_CFLAGS =$(common_cflags_clang) --compiler=I386_CLANG8
libbambu_clang8_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags_clang) --compiler=I386_CLANG8 -C
endif

if BUILD_I386_CLANG9
pkglib_LIBRARIES += libbambu_clang9.a
libbambu_clang9_a_SOURCES = $(common_libbambu_src)
libbambu_clang9_a_CFLAGS =$(common_cflags_clang) --compiler=I386_CLANG9
libbambu_clang9_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags_clang) --compiler=I386_CLANG9 -C
endif

if BUILD_I386_CLANG10
pkglib_LIBRARIES += libbambu_clang10.a
libbambu_clang10_a_SOURCES = $(common_libbambu_src)
libbambu_clang10_a_CFLAGS =$(common_cflags_clang) --compiler=I386_CLANG10
libbambu_clang10_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags_clang) --compiler=I386_CLANG10 -C
endif

if BUILD_I386_CLANG11
pkglib_LIBRARIES += libbambu_clang11.a
libbambu_clang11_a_SOURCES = $(common_libbambu_src)
libbambu_clang11_a_CFLAGS =$(common_cflags_clang) --compiler=I386_CLANG11
libbambu_clang11_a_AR =$(top_builddir)/src/tree-panda-gcc $(common_treegccflags_clang) --compiler=I386_CLANG11 -C
endif

