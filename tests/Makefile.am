#
# Copyright (C) 2015 David Bigagli
#

noinst_PROGRAMS = tests
AM_CPPFLAGS = -I$(top_srcdir)/lsf -I$(top_srcdir)/lsbatch
tests_SOURCES = tests.c libtests.c tests.h
tests_LDADD = ../lsbatch/lib/liblsbatch.la ../lsf/lib/liblsf.la \
	../lsf/intlib/liblsfint.la -lm -lnsl
