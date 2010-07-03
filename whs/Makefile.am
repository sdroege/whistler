NULL =

CLASSIFIER = \"WhsNNClassifier_32_32_32_1\"

lib_LTLIBRARIES = libwhistler.la

libwhistler_la_SOURCES = \
	whs.c \
	whsobject.c \
	whsidentifier.c \
	whslearner.c \
	whsextractor.c \
	whslocalizer.c \
	whstrainingdata.c \
	whspattern.c \
	whsclassifier.c \
	whsbandpass.c \
	classifier.c \
	classifier/whsnnclassifier32-16-1.c \
	classifier/whsnnclassifier32-32-1.c \
	classifier/whsnnclassifier32-32-32-1.c \
	$(NULL)

libwhistler_includedir = $(includedir)/whistler
libwhistler_include_HEADERS = \
	whs.h \
	whsobject.h \
	whsidentifier.h \
	whstrainingdata.h \
	whslearner.h \
	whspattern.h \
	$(NULL)

noinst_HEADERS = \
	whsextractor.h \
	whsclassifier.h \
	whslocalizer.h \
	whsutils.h \
	whsprivate.h \
	whspatternprivate.h \
	whsbandpass.h \
	classifier.h \
	classifier/whsnnclassifier32-16-1.h \
	classifier/whsnnclassifier32-32-1.h \
	classifier/whsnnclassifier32-32-32-1.h \
	$(NULL)

libwhistler_la_LIBADD = \
	$(GLIB_LIBS) \
	$(LIBM) \
	$(AM_LDADD) \
	$(top_builddir)/ext/gpfft/libgpfft.la \
	$(NULL)

libwhistler_la_LDFLAGS = \
	-export-dynamic \
	-no-undefined \
	-export-symbols-regex '^whs_.*' \
	-version-info $(WHS_VERSION_INFO) \
	$(AM_LDFLAGS) \
	$(NULL)

libwhistler_la_CFLAGS = \
	$(GLIB_CFLAGS) \
	$(GLIB_CFLAGS_EXTRA) \
	$(AM_CFLAGS) \
	-I$(top_srcdir)/ext/gpfft \
	-DCLASSIFIER=$(CLASSIFIER) \
	$(NULL)

