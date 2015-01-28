NULL=
SUBDIRS = \
	config                            \
	ferrisstreams.test                 \
	sampledata                  

noinst_PROGRAMS = \
	ut_stringstream ut_nullstream ut_limiting_istream \
	ut_proxy_streams ut_close_signal ut_fd_streams    \
	ut_radix_dump ut_tee_stream

INCLUDES =                                              \
	@STLPORT_CFLAGS@                                \
	-I. 						\
	-I$(srcdir) -I$(top_srcdir) -I$(top_srcdir)/src	\
	-I$(top_builddir)				\
        -I$(includedir)                                 \
	@GLIB_CFLAGS@                                   \
	@SIGC_CFLAGS@                                   \
	@LOKI_CFLAGS@                                   \
	@CFLAGS@

ut_stringstream_SOURCES = ut_stringstream.cpp
ut_stringstream_LDADD   = -lpopt @LIBFERRISSTREAMS_LA@ 
ut_stringstream_LDFLAGS = $(GLIB_LIBS) @SIGC_LIBS@ @LOKI_LIBS@

ut_nullstream_SOURCES = ut_nullstream.cpp
ut_nullstream_LDADD   = -lpopt @LIBFERRISSTREAMS_LA@ 
ut_nullstream_LDFLAGS = $(GLIB_LIBS) @SIGC_LIBS@ @LOKI_LIBS@

ut_limiting_istream_SOURCES = ut_limiting_istream.cpp
ut_limiting_istream_LDADD   = -lpopt @LIBFERRISSTREAMS_LA@ 
ut_limiting_istream_LDFLAGS = $(GLIB_LIBS) @SIGC_LIBS@ @LOKI_LIBS@

ut_proxy_streams_SOURCES = ut_proxy_streams.cpp
ut_proxy_streams_LDADD   = -lpopt @LIBFERRISSTREAMS_LA@ 
ut_proxy_streams_LDFLAGS = $(GLIB_LIBS) @SIGC_LIBS@ @LOKI_LIBS@

ut_close_signal_SOURCES = ut_close_signal.cpp
ut_close_signal_LDADD   = -lpopt @LIBFERRISSTREAMS_LA@ 
ut_close_signal_LDFLAGS = $(GLIB_LIBS) @SIGC_LIBS@ @LOKI_LIBS@

ut_fd_streams_SOURCES = ut_fd_streams.cpp
ut_fd_streams_LDADD   = -lpopt @LIBFERRISSTREAMS_LA@ 
ut_fd_streams_LDFLAGS = $(GLIB_LIBS) @SIGC_LIBS@ @LOKI_LIBS@

ut_radix_dump_SOURCES = ut_radix_dump.cpp
ut_radix_dump_LDADD   = -lpopt @LIBFERRISSTREAMS_LA@ 
ut_radix_dump_LDFLAGS = $(GLIB_LIBS) @SIGC_LIBS@ @LOKI_LIBS@

ut_tee_stream_SOURCES = ut_tee_stream.cpp
ut_tee_stream_LDADD   = -lpopt @LIBFERRISSTREAMS_LA@ 
ut_tee_stream_LDFLAGS = $(GLIB_LIBS) @SIGC_LIBS@ @LOKI_LIBS@

