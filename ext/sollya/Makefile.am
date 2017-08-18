SUBDIRS = doc tests
EXTRA_DIST = autogen.sh sollya.h libsollyaexample.c compile.sh expansion.c expansion.h externalfun.c externalproc.c 

check_PROGRAMS = sollya

AM_YFLAGS = -d
AM_LFLAGS = -o$(LEX_OUTPUT_ROOT).c

BUILT_SOURCES = \
miniparser.h \
parser.h \
internparser.h \
lexer.c \
minilexer.c \
internlexer.c 

bin_PROGRAMS = sollya
sollya_SOURCES = \
mpfi-compat.c \
mpfi-compat.h \
internparser.y \
miniparser.y \
parser.y \
internlexer.l \
lexer.l \
minilexer.l \
help.h \
assignment.c \
assignment.h \
autodiff.c \
autodiff.h \
chain.c \
chain.h \
double.c \
double.h \
execute.c \
execute.h \
expression.c \
expression.h \
external.c \
external.h \
general.c \
general.h \
implement.c \
implement.h \
implementconst.c \
implementconst.h \
infnorm.c \
infnorm.h \
integral.c \
integral.h \
library.c \
library.h \
main.c \
main.h \
plot.c \
plot.h \
proof.c \
proof.h \
fpminimax.cpp \
fpminimax.h \
remez.c \
remez.h \
match.c \
match.h \
taylor.c \
taylor.h \
taylorform.c \
taylorform.h \
supnorm.c \
supnorm.h \
sturm.c \
sturm.h \
worstcase.c \
worstcase.h \
xml.c \
xml.h 
sollya_CFLAGS = $(AM_CFLAGS)
sollya_CXXFLAGS = $(AM_CXXFLAGS)

include_HEADERS = sollya.h mpfi-compat.h

lib_LTLIBRARIES     = libsollya.la
libsollya_la_SOURCES = \
sollya.h \
mpfi-compat.h \
mpfi-compat.c \
internparser.y \
miniparser.y \
parser.y \
internlexer.l \
lexer.l \
minilexer.l \
help.h \
assignment.c \
assignment.h \
autodiff.c \
autodiff.h \
chain.c \
chain.h \
double.c \
double.h \
execute.c \
execute.h \
expression.c \
expression.h \
external.c \
external.h \
general.c \
general.h \
implement.c \
implement.h \
implementconst.c \
implementconst.h \
infnorm.c \
infnorm.h \
integral.c \
integral.h \
library.c \
library.h \
plot.c \
plot.h \
proof.c \
proof.h \
fpminimax.cpp \
fpminimax.h \
remez.c \
remez.h \
match.c \
match.h \
taylorform.c \
taylorform.h \
supnorm.c \
supnorm.h \
sturm.c \
sturm.h \
taylor.c \
taylor.h \
worstcase.c \
worstcase.h \
xml.c \
xml.h 


LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

