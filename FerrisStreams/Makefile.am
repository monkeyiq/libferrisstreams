
MAINTAINERCLEANFILES = Makefile.in
#EXTRA_DIST = 


INCLUDES =                                              \
	@STLPORT_CFLAGS@                                \
	-I. 						\
	-I$(srcdir) -I$(top_srcdir) 			\
	-I$(top_builddir)				\
	@SIGC_CFLAGS@                                   \
	@LOKI_CFLAGS@                                   \
	@CXXFLAGS@


lib_LTLIBRARIES      = libferrisstreams.la

libferrisstreams_includedir=$(includedir)/FerrisStreams
libferrisstreams_include_HEADERS = \
	Streams.hh Exceptions.hh Shell.hh All.hh HiddenSymbolSupport.hh FerrisPosix.hh

libferrisstreams_la_SOURCES = \
	Streams.cpp Exceptions.cpp Shell.cpp FerrisPosix.cpp

libferrisstreams_la_LIBADD  = @SIGC_LIBS@ @LOKI_LIBS@
libferrisstreams_la_LDFLAGS = @VERSION_INFO@ 

