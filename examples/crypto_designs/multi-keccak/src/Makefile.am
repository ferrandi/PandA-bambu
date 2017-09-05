.NOTPARALLEL:
SUBDIRS = chi iota pi rho theta

bin_PROGRAMS = keccak
keccak_SOURCES = keccak_coproc.c
keccak_CPPFLAGS = -I$(top_srcdir)/include
keccak_LDADD = chi/libchi.a iota/libiota.a pi/libpi.a rho/librho.a theta/libtheta.a


