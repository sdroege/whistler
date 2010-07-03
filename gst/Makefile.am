NULL=

plugin_LTLIBRARIES = libwhistler-gst.la
plugindir=$(GSTREAMER_PLUGINS_DIR)

libwhistler_gst_la_SOURCES = \
	whsgstidentifier.c \
	whsgstlearner.c \
	plugin.c \
	$(NULL)

libwhistler_gst_la_CFLAGS = \
	$(GSTREAMER_CFLAGS) \
	-I$(top_srcdir) \
	$(NULL)

libwhistler_gst_la_LIBADD = \
	$(GSTREAMER_LIBS) \
	$(LIBM) \
	$(top_builddir)/whs/.libs/libwhistler.la \
	$(NULL)

libwhistler_gst_la_LDFLAGS = \
	-module \
	-avoid-version \
	-export-symbols-regex '^_*gst_plugin_desc$$' \
	-no-undefined \
	$(NULL)

noinst_HEADERS = \
	whsgstidentifier.h \
	whsgstlearner.h \
	$(NULL)

