noinst_HEADERS += \
	tests/dummy/patchentrypoints.h \
	tests/dummy/GLX_dummy.h \
	tests/dummy/EGL_dummy.h

if ENABLE_GLX
check_LTLIBRARIES += libGLX_dummy.la
libGLX_dummy_la_CFLAGS = \
	-I$(top_srcdir)/src/GLX        \
	-I$(top_srcdir)/src/GLdispatch \
	-I$(top_srcdir)/src/util       \
	-I$(top_srcdir)/include
libGLX_dummy_la_SOURCES =
libGLX_dummy_la_SOURCES += tests/dummy/GLX_dummy.c
libGLX_dummy_la_SOURCES += src/util/trace.c
libGLX_dummy_la_SOURCES += src/util/utils_misc.c
libGLX_dummy_la_SOURCES += tests/dummy/patchentrypoints.c
# The -rpath option forces libtool to create a shared library instead of just a
# static library.
libGLX_dummy_la_LDFLAGS = \
	-shared \
	-rpath /nowhere
endif # ENABLE_GLX

if ENABLE_EGL
EGL_DUMMY_CFLAGS_COMMON = \
	-I$(top_srcdir)/src/util \
	-I$(top_srcdir)/include
EGL_DUMMY_SOURCES_COMMON =
EGL_DUMMY_SOURCES_COMMON += tests/dummy/EGL_dummy.c
EGL_DUMMY_SOURCES_COMMON += src/util/glvnd_pthread.c
EGL_DUMMY_LDFLAGS_COMMON = \
	-shared \
	-rpath /nowhere

check_LTLIBRARIES += libEGL_dummy0.la
libEGL_dummy0_la_CFLAGS = \
	$(EGL_DUMMY_CFLAGS_COMMON) \
	-D DUMMY_VENDOR_NAME=\"dummy0\"
libEGL_dummy0_la_SOURCES = $(EGL_DUMMY_SOURCES_COMMON)
libEGL_dummy0_la_LIBADD = $(EGL_DUMMY_LIBADD_COMMON)
libEGL_dummy0_la_LDFLAGS = $(EGL_DUMMY_LDFLAGS_COMMON)

check_LTLIBRARIES += libEGL_dummy1.la
libEGL_dummy1_la_CFLAGS = \
	$(EGL_DUMMY_CFLAGS_COMMON) \
	-D DUMMY_VENDOR_NAME=\"dummy1\"
libEGL_dummy1_la_SOURCES = $(EGL_DUMMY_SOURCES_COMMON)
libEGL_dummy1_la_LIBADD = $(EGL_DUMMY_LIBADD_COMMON)
libEGL_dummy1_la_LDFLAGS = $(EGL_DUMMY_LDFLAGS_COMMON)

endif # ENABLE_EGL
