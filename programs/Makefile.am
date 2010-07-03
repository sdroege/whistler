NULL =

libraries = \
	$(GLIB_LIBS) \
	$(top_builddir)/whs/.libs/libwhistler.la \
	$(AM_LDADD) \
	$(NULL)

cflags = \
	$(GLIB_CFLAGS) \
	$(GLIB_CFLAGS_EXTRA) \
	-I$(top_srcdir) \
	$(AM_CFLAGS) \
	$(NULL)

bin_PROGRAMS = \
	whs-learn \
	$(NULL)

whs_learn_SOURCES = learn.c
whs_learn_LDADD = $(libraries)
whs_learn_CFLAGS = $(cflags)

