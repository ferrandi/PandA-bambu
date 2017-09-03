#this is file src/HLS/bindind/Makefile.am
METASOURCES = AUTO

noinst_LTLIBRARIES = lib_binding.la

SUBDIRS = register interconnection module storage_value_insertion

noinst_HEADERS =
lib_binding_la_SOURCES =

lib_binding_la_LIBADD = $(top_builddir)/src/HLS/binding/register/lib_registerbinding.la \
                        $(top_builddir)/src/HLS/binding/interconnection/lib_interconnectionbinding.la \
                        $(top_builddir)/src/HLS/binding/storage_value_insertion/lib_storage_value_insertion.la \
                        $(top_builddir)/src/HLS/binding/module/lib_modulebinding.la

#DOCUMENTATION STUFF

DOX_TAGFILES= src src/HLS src/HLS/binding/register src/HLS/binding/storage_value_insertion src/HLS/binding/module src/HLS/binding/interconnection

PRJ_DOC = binding.doc
EXTRA_DIST = $(PRJ_DOC)

#do not touch the following line

