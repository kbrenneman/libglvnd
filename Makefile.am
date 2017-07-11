ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST =
noinst_LTLIBRARIES=
lib_LTLIBRARIES=
EXTRA_LTLIBRARIES=
check_LTLIBRARIES=
noinst_HEADERS=
BUILT_SOURCES=
CLEANFILES=

include src/generate/glvnd_gen.mk

include src/util/Makefile.am
include src/GLdispatch/Makefile.am
include src/OpenGL/Makefile.am
include src/GLESv1/Makefile.am
include src/GLESv2/Makefile.am

if ENABLE_GLX
include src/GLX/Makefile.am
include src/GL/Makefile.am
endif

if ENABLE_EGL
include src/EGL/Makefile.am
endif

include tests/Makefile.am

EXTRA_DIST += autogen.sh README.md
clean-local:
	rm -f $(top_srcdir)/src/generate/*.pyc

# Add the files under the include/ subdirectory.
glvndinclude_HEADERS = \
	include/glvnd/GLdispatchABI.h \
	include/glvnd/libglxabi.h \
	include/glvnd/libeglabi.h
glvndincludedir = $(includedir)/glvnd
noinst_HEADERS += include

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libglvnd.pc

CLEANFILES += $(BUILT_SOURCES)
CLEANFILES += $(EXTRA_LTLIBRARIES)
