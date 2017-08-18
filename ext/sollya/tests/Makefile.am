EXTRA_DIST= check.proto timecheck.sollya timecheck.sollya.reference check001.sollya check002.sollya check003.sollya check004.sollya check005.sollya check006.sollya check007.sollya check008.sollya check009.sollya check010.sollya check011.sollya check012.sollya check013.sollya check014.sollya check015.sollya check016.sollya check017.sollya check018.sollya check001.sollya.reference check002.sollya.reference check003.sollya.reference check004.sollya.reference check005.sollya.reference check006.sollya.reference check007.sollya.reference check008.sollya.reference check009.sollya.reference check010.sollya.reference check011.sollya.reference check012.sollya.reference check013.sollya.reference check014.sollya.reference check015.sollya.reference check016.sollya.reference check017.sollya.reference check018.sollya.reference

TESTS_ENVIRONMENT=$(SHELL) 
#TESTS= check.sh 

TESTFILES= timecheck.sollya check001.sollya check002.sollya check003.sollya check004.sollya check005.sollya check006.sollya check007.sollya check008.sollya check009.sollya check010.sollya check011.sollya check012.sollya check013.sollya check014.sollya check015.sollya check016.sollya check017.sollya check018.sollya

.PHONY: $(top_builddir)/sollya

$(top_builddir)/sollya:
	cd $(top_builddir); make all


check.sh: $(top_builddir)/sollya check.proto $(TESTFILES)
	echo "#!/bin/sh" > check.sh
	echo "SOLLYA="$(top_builddir)"/sollya" >> check.sh
	echo "TESTFILES=\""$(TESTFILES)"\"" >> check.sh
	cat check.proto >> check.sh
	chmod +x check.sh






