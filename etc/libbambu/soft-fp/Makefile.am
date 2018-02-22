#this file is libbambu/libsoft-fp/Makefile.am
EXTRA_DIST=bambu-arch.h double.h extended.h longlong.h op-1.h op-2.h op-4.h op-8.h op-common.h quad.h sfp-machine.h single.h soft-fp.h common_core_nonrestoring_sdiv.h README
common_treegccflags = --output-temporary-directory=/tmp -iplugindir=$(top_builddir)/etc/gcc_plugin
common_cflags= -O3 -D_FLT_NO_DENORMALS -I$(top_srcdir)/etc/libbambu -fipa-pta $(common_treegccflags)
common_cflags_nr= -O2 -D_FLT_NO_DENORMALS -I$(top_srcdir)/etc/libbambu -fipa-pta -finline-functions $(common_treegccflags)

METASOURCES = AUTO
CC=$(top_builddir)/src/tree-panda-gcc
CFLAGS=
AM_CFLAGS=
pkglib_LIBRARIES = 
common_libsoft_fp_src = adddf3.c addsf3.c divdf3.c divsf3.c eqdf2.c eqsf2.c extendsfdf2.c \
  fixdfdi.c fixdfsi.c fixsfdi.c fixsfsi.c fixunsdfdi.c\
  fixunsdfsi.c fixunssfdi.c fixunssfsi.c floatdidf.c floatdisf.c\
  floatsidf.c floatsisf.c floatundidf.c floatundisf.c\
  floatunsidf.c floatunsisf.c gedf2.c gesf2.c ledf2.c lesf2.c\
  muldf3.c mulsf3.c negdf2.c negsf2.c subdf3.c subsf3.c truncdfsf2.c\
  unorddf2.c unordsf2.c 

#addtf3.c divtf3.c eqtf2.c extenddftf2.c fixtfdi.c fixtfsi.c fixunstfdi.c fixunstfsi.c floatditf.c floatsitf.c floatunditf.c floatunsitf.c getf2.c letf2.c multf3.c negtf2.c subtf3.c trunctfdf2.c trunctfsf2.c unordtf2.c extendsftf2.c
#fixdfti.c  fixsfti.c fixtfti.c fixunsdfti.c fixunssfti.c fixunstfti.c floattidf.c floattisf.c floattitf.c floatuntidf.c floatuntisf.c floatuntitf.c extendxftf2.c trunctfxf2.c
common_libhls_div_src = divsi3.c divdi3.c divdi36432.c
common_libhls_divnr_src = divsi3nr.c divdi3nr.c divdi3nr6432.c
common_libhls_divas_src = divsi3as.c divdi3as.c divdi3as6432.c
common_libhls_cdiv_src = divsc3.c divdc3.c

if BUILD_I386_GCC45
pkglib_LIBRARIES += libsoft-fp_gcc45.a libhls-divNR_gcc45.a libhls-divnr1_gcc45.a libhls-divnr2_gcc45.a libhls-divas_gcc45.a libhls-cdiv_gcc45.a
libsoft_fp_gcc45_a_SOURCES = $(common_libsoft_fp_src)
libsoft_fp_gcc45_a_CFLAGS = --compiler=I386_GCC45 $(common_cflags) -I$(BOOST_DIR)
libsoft_fp_gcc45_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC45 $(common_treegccflags) -C
libhls_divNR_gcc45_a_SOURCES = $(common_libhls_div_src)
libhls_divNR_gcc45_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC45  $(common_cflags_nr) -I$(BOOST_DIR)
libhls_divNR_gcc45_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC45 $(common_treegccflags) -C
libhls_divnr1_gcc45_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr1_gcc45_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC45  $(common_cflags) -I$(BOOST_DIR) -DUNROLL_FACTOR=1
libhls_divnr1_gcc45_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC45 $(common_treegccflags) -C
libhls_divnr2_gcc45_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr2_gcc45_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC45  $(common_cflags) -I$(BOOST_DIR) -DUNROLL_FACTOR=2
libhls_divnr2_gcc45_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC45 $(common_treegccflags) -C
libhls_divas_gcc45_a_SOURCES = $(common_libhls_divas_src)
libhls_divas_gcc45_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC45  $(common_cflags) -I$(BOOST_DIR)
libhls_divas_gcc45_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC45 $(common_treegccflags) -C
libhls_cdiv_gcc45_a_SOURCES = $(common_libhls_cdiv_src)
libhls_cdiv_gcc45_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC45  $(common_cflags) -I$(BOOST_DIR)
libhls_cdiv_gcc45_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC45 $(common_treegccflags) -C

endif

if BUILD_I386_GCC46
pkglib_LIBRARIES += libsoft-fp_gcc46.a libhls-divNR_gcc46.a libhls-divnr1_gcc46.a libhls-divnr2_gcc46.a libhls-divas_gcc46.a  libhls-cdiv_gcc46.a
libsoft_fp_gcc46_a_SOURCES = $(common_libsoft_fp_src)
libsoft_fp_gcc46_a_CFLAGS =  --compiler=I386_GCC46  $(common_cflags)  -I$(BOOST_DIR)
libsoft_fp_gcc46_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC46 $(common_treegccflags) -C
libhls_divNR_gcc46_a_SOURCES = $(common_libhls_div_src)
libhls_divNR_gcc46_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC46  $(common_cflags_nr)  -I$(BOOST_DIR)
libhls_divNR_gcc46_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC46 $(common_treegccflags) -C
libhls_divnr1_gcc46_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr1_gcc46_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC46  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=1
libhls_divnr1_gcc46_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC46 $(common_treegccflags) -C
libhls_divnr2_gcc46_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr2_gcc46_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC46  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=2
libhls_divnr2_gcc46_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC46 $(common_treegccflags) -C 
libhls_divas_gcc46_a_SOURCES = $(common_libhls_divas_src)
libhls_divas_gcc46_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC46  $(common_cflags)  -I$(BOOST_DIR)
libhls_divas_gcc46_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC46 $(common_treegccflags) -C
libhls_cdiv_gcc46_a_SOURCES = $(common_libhls_cdiv_src)
libhls_cdiv_gcc46_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC46  $(common_cflags)  -I$(BOOST_DIR)
libhls_cdiv_gcc46_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC46 $(common_treegccflags) -C
endif

if BUILD_I386_GCC47 
pkglib_LIBRARIES += libsoft-fp_gcc47.a libhls-divNR_gcc47.a libhls-divnr1_gcc47.a libhls-divnr2_gcc47.a libhls-divas_gcc47.a  libhls-cdiv_gcc47.a
libsoft_fp_gcc47_a_SOURCES = $(common_libsoft_fp_src)
libsoft_fp_gcc47_a_CFLAGS =  --compiler=I386_GCC47  $(common_cflags) -I$(BOOST_DIR) 
libsoft_fp_gcc47_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC47 $(common_treegccflags) -C
libhls_divNR_gcc47_a_SOURCES = $(common_libhls_div_src)
libhls_divNR_gcc47_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC47  $(common_cflags_nr) -I$(BOOST_DIR)
libhls_divNR_gcc47_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC47 $(common_treegccflags) -C
libhls_divnr1_gcc47_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr1_gcc47_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC47  $(common_cflags) -I$(BOOST_DIR) -DUNROLL_FACTOR=1
libhls_divnr1_gcc47_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC47 $(common_treegccflags) -C
libhls_divnr2_gcc47_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr2_gcc47_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC47  $(common_cflags) -I$(BOOST_DIR) -DUNROLL_FACTOR=2
libhls_divnr2_gcc47_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC47 $(common_treegccflags) -C
libhls_divas_gcc47_a_SOURCES = $(common_libhls_divas_src)
libhls_divas_gcc47_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC47  $(common_cflags) -I$(BOOST_DIR)
libhls_divas_gcc47_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC47 $(common_treegccflags) -C
libhls_cdiv_gcc47_a_SOURCES = $(common_libhls_cdiv_src)
libhls_cdiv_gcc47_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC47  $(common_cflags) -I$(BOOST_DIR)
libhls_cdiv_gcc47_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC47 $(common_treegccflags) -C
endif

if BUILD_I386_GCC48
pkglib_LIBRARIES += libsoft-fp_gcc48.a libhls-divNR_gcc48.a libhls-divnr1_gcc48.a libhls-divnr2_gcc48.a libhls-divas_gcc48.a  libhls-cdiv_gcc48.a
libsoft_fp_gcc48_a_SOURCES = $(common_libsoft_fp_src)
libsoft_fp_gcc48_a_CFLAGS =  --compiler=I386_GCC48  $(common_cflags) -I$(BOOST_DIR)
libsoft_fp_gcc48_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC48 $(common_treegccflags) -C
libhls_divNR_gcc48_a_SOURCES = $(common_libhls_div_src)
libhls_divNR_gcc48_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC48  $(common_cflags_nr) -I$(BOOST_DIR)
libhls_divNR_gcc48_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC48 $(common_treegccflags) -C
libhls_divnr1_gcc48_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr1_gcc48_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC48  $(common_cflags) -I$(BOOST_DIR) -DUNROLL_FACTOR=1
libhls_divnr1_gcc48_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC48 $(common_treegccflags) -C
libhls_divnr2_gcc48_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr2_gcc48_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC48  $(common_cflags) -I$(BOOST_DIR) -DUNROLL_FACTOR=2
libhls_divnr2_gcc48_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC48 $(common_treegccflags) -C 
libhls_divas_gcc48_a_SOURCES = $(common_libhls_divas_src)
libhls_divas_gcc48_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC48  $(common_cflags) -I$(BOOST_DIR)
libhls_divas_gcc48_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC48 $(common_treegccflags) -C
libhls_cdiv_gcc48_a_SOURCES = $(common_libhls_cdiv_src)
libhls_cdiv_gcc48_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC48  $(common_cflags) -I$(BOOST_DIR)
libhls_cdiv_gcc48_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC48 $(common_treegccflags) -C
endif

if BUILD_I386_GCC49
pkglib_LIBRARIES += libsoft-fp_gcc49.a libhls-divNR_gcc49.a libhls-divnr1_gcc49.a libhls-divnr2_gcc49.a libhls-divas_gcc49.a  libhls-cdiv_gcc49.a
libsoft_fp_gcc49_a_SOURCES = $(common_libsoft_fp_src)
libsoft_fp_gcc49_a_CFLAGS = --compiler=I386_GCC49  $(common_cflags)  -I$(BOOST_DIR)
libsoft_fp_gcc49_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C
libhls_divNR_gcc49_a_SOURCES = $(common_libhls_div_src)
libhls_divNR_gcc49_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC49  $(common_cflags_nr)  -I$(BOOST_DIR)
libhls_divNR_gcc49_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C
libhls_divnr1_gcc49_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr1_gcc49_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC49  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=1
libhls_divnr1_gcc49_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C
libhls_divnr2_gcc49_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr2_gcc49_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC49  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=2
libhls_divnr2_gcc49_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C 
libhls_divas_gcc49_a_SOURCES = $(common_libhls_divas_src)
libhls_divas_gcc49_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC49  $(common_cflags)  -I$(BOOST_DIR)
libhls_divas_gcc49_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C
libhls_cdiv_gcc49_a_SOURCES = $(common_libhls_cdiv_src)
libhls_cdiv_gcc49_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC49  $(common_cflags)  -I$(BOOST_DIR)
libhls_cdiv_gcc49_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC49 $(common_treegccflags) -C
endif

if BUILD_I386_GCC5
pkglib_LIBRARIES += libsoft-fp_gcc5.a libhls-divNR_gcc5.a libhls-divnr1_gcc5.a libhls-divnr2_gcc5.a libhls-divas_gcc5.a  libhls-cdiv_gcc5.a
libsoft_fp_gcc5_a_SOURCES = $(common_libsoft_fp_src)
libsoft_fp_gcc5_a_CFLAGS = --compiler=I386_GCC5  $(common_cflags)  -I$(BOOST_DIR)
libsoft_fp_gcc5_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C
libhls_divNR_gcc5_a_SOURCES = $(common_libhls_div_src)
libhls_divNR_gcc5_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC5  $(common_cflags_nr)  -I$(BOOST_DIR)
libhls_divNR_gcc5_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C
libhls_divnr1_gcc5_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr1_gcc5_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC5  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=1
libhls_divnr1_gcc5_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C
libhls_divnr2_gcc5_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr2_gcc5_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC5  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=2
libhls_divnr2_gcc5_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C 
libhls_divas_gcc5_a_SOURCES = $(common_libhls_divas_src)
libhls_divas_gcc5_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC5  $(common_cflags)  -I$(BOOST_DIR)
libhls_divas_gcc5_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C
libhls_cdiv_gcc5_a_SOURCES = $(common_libhls_cdiv_src)
libhls_cdiv_gcc5_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC5  $(common_cflags)  -I$(BOOST_DIR)
libhls_cdiv_gcc5_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC5 $(common_treegccflags) -C
endif

if BUILD_I386_GCC6
pkglib_LIBRARIES += libsoft-fp_gcc6.a libhls-divNR_gcc6.a libhls-divnr1_gcc6.a libhls-divnr2_gcc6.a libhls-divas_gcc6.a  libhls-cdiv_gcc6.a
libsoft_fp_gcc6_a_SOURCES = $(common_libsoft_fp_src)
libsoft_fp_gcc6_a_CFLAGS = --compiler=I386_GCC6  $(common_cflags)  -I$(BOOST_DIR)
libsoft_fp_gcc6_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C
libhls_divNR_gcc6_a_SOURCES = $(common_libhls_div_src)
libhls_divNR_gcc6_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC6  $(common_cflags_nr)  -I$(BOOST_DIR)
libhls_divNR_gcc6_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C
libhls_divnr1_gcc6_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr1_gcc6_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC6  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=1
libhls_divnr1_gcc6_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C
libhls_divnr2_gcc6_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr2_gcc6_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC6  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=2
libhls_divnr2_gcc6_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C 
libhls_divas_gcc6_a_SOURCES = $(common_libhls_divas_src)
libhls_divas_gcc6_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC6  $(common_cflags)  -I$(BOOST_DIR)
libhls_divas_gcc6_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C
libhls_cdiv_gcc6_a_SOURCES = $(common_libhls_cdiv_src)
libhls_cdiv_gcc6_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC6  $(common_cflags)  -I$(BOOST_DIR)
libhls_cdiv_gcc6_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC6 $(common_treegccflags) -C
endif

if BUILD_I386_GCC7
pkglib_LIBRARIES += libsoft-fp_gcc7.a libhls-divNR_gcc7.a libhls-divnr1_gcc7.a libhls-divnr2_gcc7.a libhls-divas_gcc7.a  libhls-cdiv_gcc7.a
libsoft_fp_gcc7_a_SOURCES = $(common_libsoft_fp_src)
libsoft_fp_gcc7_a_CFLAGS = --compiler=I386_GCC7  $(common_cflags)  -I$(BOOST_DIR)
libsoft_fp_gcc7_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
libhls_divNR_gcc7_a_SOURCES = $(common_libhls_div_src)
libhls_divNR_gcc7_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC7  $(common_cflags_nr)  -I$(BOOST_DIR)
libhls_divNR_gcc7_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
libhls_divnr1_gcc7_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr1_gcc7_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC7  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=1
libhls_divnr1_gcc7_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
libhls_divnr2_gcc7_a_SOURCES = $(common_libhls_divnr_src)
libhls_divnr2_gcc7_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC7  $(common_cflags)  -I$(BOOST_DIR) -DUNROLL_FACTOR=2
libhls_divnr2_gcc7_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
libhls_divas_gcc7_a_SOURCES = $(common_libhls_divas_src)
libhls_divas_gcc7_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC7  $(common_cflags)  -I$(BOOST_DIR)
libhls_divas_gcc7_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
libhls_cdiv_gcc7_a_SOURCES = $(common_libhls_cdiv_src)
libhls_cdiv_gcc7_a_CFLAGS = -finline-limit=10000 --compiler=I386_GCC7  $(common_cflags)  -I$(BOOST_DIR)
libhls_cdiv_gcc7_a_AR = $(top_builddir)/src/tree-panda-gcc --compiler=I386_GCC7 $(common_treegccflags) -C
endif

