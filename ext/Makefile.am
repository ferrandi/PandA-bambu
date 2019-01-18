EXTRA_DIST = GRLIB/grlib/amba/amba.vhd GRLIB/grlib/amba/devices.vhd GRLIB/grlib/amba/ahbctrl.vhd GRLIB/grlib/amba/apbctrl.vhd GRLIB/grlib/stdlib/stdlib.vhd GRLIB/grlib/stdlib/version.vhd GRLIB/techmap/maps/inpad.vhd GRLIB/techmap/maps/outpad.vhd GRLIB/techmap/maps/clkpad.vhd GRLIB/techmap/maps/iopad.vhd GRLIB/techmap/maps/allclkgen.vhd GRLIB/techmap/maps/iodpad.vhd GRLIB/techmap/maps/toutpad.vhd GRLIB/techmap/maps/allpads.vhd GRLIB/techmap/maps/odpad.vhd GRLIB/techmap/maps/clkgen.vhd GRLIB/techmap/gencomp GRLIB/techmap/gencomp/gencomp.vhd GRLIB/techmap/unisim/clkgen_unisim.vhd GRLIB/techmap/unisim/pads_unisim.vhd GRLIB/gaisler/misc/misc.vhd GRLIB/gaisler/misc/ahbmst.vhd GRLIB/gaisler/misc/rstgen.vhd GRLIB/gaisler/pci/pci.vhd GRLIB/gaisler/pci/pci_target.vhd GRLIB/gaisler/pci/pcipads.vhd

SUBDIRS =
DIST_SUBDIRS =

if BUILD_ABC
   noinst_SCRIPTS =create_abc
endif

if BUILD_BEAGLE
   SUBDIRS += beagle
endif

if BUILD_COIN_OR
   SUBDIRS += Coin-Cbc
   lib_LTLIBRARIES = libCoin_Cbc.la
   libCoin_Cbc_la_SOURCES =
   libCoin_Cbc_la_LIBADD = Coin-Cbc/Cbc/src/libCbcSolver.la \
      Coin-Cbc/Cbc/src/OsiCbc/libOsiCbc.la \
      Coin-Cbc/Cbc/src/libCbc.la \
      Coin-Cbc/Clp/src/OsiClp/libOsiClp.la \
      Coin-Cbc/Cgl/src/libCgl.la \
      Coin-Cbc/Clp/src/libClp.la \
      Coin-Cbc/Osi/src/Osi/libOsi.la \
      Coin-Cbc/CoinUtils/src/libCoinUtils.la
endif

if BUILD_CROSSTOOL_NG
   SUBDIRS += crosstool-ng
endif

if BUILD_METSLIB
   SUBDIRS += metslib-0.5.3
   DIST_SUBDIRS += metslib-0.5.3
endif

if BUILD_TRNG
   SUBDIRS += trng-4.17
   DIST_SUBDIRS += trng-4.17
endif


if BUILD_NODELIB
   SUBDIRS += nodelib
endif

if BUILD_LIBERTY_PARSER
   SUBDIRS += liberty_parser
endif

if BUILD_LP_SOLVE
   SUBDIRS += lpsolve5
endif

if BUILD_SIMIT
   SUBDIRS += SimIt-PoliMi
endif

if BUILD_FLOPOCO
   SUBDIRS += flopoco
   DIST_SUBDIRS += sollya flopoco libfplll-4.0.3
endif

if BUILD_VFLIB
   SUBDIRS += vflib
endif

CLEANFILES = $(noinst_SCRIPT)

if BUILD_LIB_CUDD
   SUBDIRS += cudd
endif

create_abc:Makefile
	touch create_abc
	tar xzf $(top_srcdir)/abc70930.tar.gz
	echo "cd abc70930; make"> create_abc
	chmod +x create_abc
	./create_abc

clean-local:
	-rm -rf abc

.PHONY documentation: documentation-recursive
.PHONY documentation-tag:documentation-tag-recursive
documentation: 
documentation-tag:

if USE_GRLIB
install-exec-hook:
	cp -r $(top_srcdir)/ext/GRLIB  $(libdir)
endif
