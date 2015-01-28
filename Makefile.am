AUTOMAKE_OPTIONS = dist-bzip2 subdir-objects dejagnu

NULL=
SUBDIRS = FerrisStreams testsuite

EXTRA_DIST = ferrisstreams.spec ferrisstreams.spec.in ferrisstreams.pc.in

bin_SCRIPTS = ferrisstreams-config

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = ferrisstreams.pc

RUNTESTDEFAULTFLAGS = \
	--tool ferrisstreams EXBASE=`pwd`/ SDATA=`pwd`/testsuite/sampledata --srcdir $$srcdir/testsuite
