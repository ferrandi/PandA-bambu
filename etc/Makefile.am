SUBDIRS = devices macros scripts
DIST_SUBDIRS = devices macros scripts

if BUILD_GCC_PLUGIN
   SUBDIRS += gcc_plugin
   DIST_SUBDIRS += gcc_plugin
endif

if BUILD_LIBBAMBU
   SUBDIRS += lib libbambu
   DIST_SUBDIRS += lib libbambu
endif

if BUILD_LATEX_TABLE_FORMAT
   SUBDIRS += latex_table_format
   DIST_SUBDIRS += latex_table_format
endif

EXTRA_DIST = Doxyfile
#Replace acinclude.m4
ACLOCAL_AMFLAGS = -I ../etc/macros

.PHONY documentation: documentation-recursive
.PHONY documentation-tag:documentation-tag-recursive
documentation:
documentation-tag:

dist-hook:
	$(top_srcdir)/scripts/clean_configure.py $(top_srcdir)/configure.ac  | grep -v "#no_release_tag" > $(distdir)/configure.ac

