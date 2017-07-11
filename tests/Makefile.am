# Copyright (c) 2013, NVIDIA CORPORATION.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# unaltered in all copies or substantial portions of the Materials.
# Any additions, deletions, or changes to the original source files
# must be clearly indicated in accompanying documentation.
#
# If only executable code is distributed, then the accompanying
# documentation must state that "this software is based in part on the
# work of the Khronos Group."
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

include tests/dummy/Makefile.am

noinst_HEADERS += \
	tests/test_utils.h \
	tests/egl_test_utils.h

TESTS_ENVIRONMENT = \
	TOP_SRCDIR=$(top_srcdir) \
	TOP_BUILDDIR=$(top_builddir) \
	ABS_TOP_BUILDDIR=$(abs_top_builddir)

TESTS =
check_PROGRAMS =

EXTRA_DIST += $(TESTS) \
	tests/glxenv.sh \
	tests/eglenv.sh \
	tests/json

# The *_oldlink variant tests that linking against legacy libGL.so works

# Disable annoying "unused" errors
TESTS_COMMON_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/util


TESTS += tests/testgldispatch_static.sh
TESTS += tests/testgldispatch_generated.sh
TESTS += tests/testgldispatch_patched.sh
check_PROGRAMS += testgldispatch
testgldispatch_SOURCES = \
	tests/testgldispatch.c \
	src/util/utils_misc.c \
	tests/dummy/patchentrypoints.c
testgldispatch_CFLAGS = $(TESTS_COMMON_CFLAGS) \
	-I$(top_srcdir)/src/GLdispatch
testgldispatch_LDADD = libGLdispatch.la
testgldispatch_LDADD += libOpenGL.la

# Start of GLX-specific tests.
# Notes that the TESTS_GLX variable must be defined outside the conditional, so
# that we can include the test scripts in the EXTRA_DIST package. Otherwise,
# the scripts would be missing when you run "make dist" or "make distcheck".

TESTS_GLX =
TESTS_GLX += tests/testglxmcbasic.sh
TESTS_GLX += tests/testglxmcloop.sh
TESTS_GLX += tests/testglxmcthreads.sh
TESTS_GLX += tests/testglxmclate.sh
TESTS_GLX += tests/testglxmcoldlink.sh
TESTS_GLX += tests/testglxgetprocaddress.sh
TESTS_GLX += tests/testglxgetprocaddress_genentry.sh
TESTS_GLX += tests/testglxgetclientstr.sh
TESTS_GLX += tests/testglxqueryversion.sh
TESTS_GLX += tests/testpatchentrypoints.sh

if ENABLE_GLX

TESTS += $(TESTS_GLX)

TESTGLXMAKECURRENT_SOURCES_COMMON = \
	tests/testglxmakecurrent.c \
	tests/test_utils.c \
	src/util/glvnd_pthread.c \
	src/util/trace.c

check_PROGRAMS += testglxmakecurrent
testglxmakecurrent_SOURCES = $(TESTGLXMAKECURRENT_SOURCES_COMMON)
testglxmakecurrent_LDADD = -lX11 -ldl
testglxmakecurrent_LDADD += libGLX.la
testglxmakecurrent_LDADD += libOpenGL.la
testglxmakecurrent_CFLAGS = $(TESTS_COMMON_CFLAGS)


check_PROGRAMS += testglxmakecurrent_oldlink
testglxmakecurrent_oldlink_SOURCES = $(TESTGLXMAKECURRENT_SOURCES_COMMON)
testglxmakecurrent_oldlink_LDADD = -lX11 -ldl
testglxmakecurrent_oldlink_LDADD += libGL.la
testglxmakecurrent_oldlink_CFLAGS = $(TESTS_COMMON_CFLAGS)


check_PROGRAMS += testglxgetprocaddress
testglxgetprocaddress_SOURCES = tests/testglxgetprocaddress.c
testglxgetprocaddress_CFLAGS = $(TESTS_COMMON_CFLAGS)
testglxgetprocaddress_LDADD = -lX11
testglxgetprocaddress_LDADD += libGLX.la


check_PROGRAMS += testglxgetclientstr
testglxgetclientstr_SOURCES = tests/testglxgetclientstr.c src/util/trace.c
testglxgetclientstr_LDADD = -lX11
testglxgetclientstr_LDADD += libGLX.la
testglxgetclientstr_LDADD += libOpenGL.la
testglxgetclientstr_CFLAGS = $(TESTS_COMMON_CFLAGS)


check_PROGRAMS += testglxqueryversion
testglxqueryversion_SOURCES = tests/testglxqueryversion.c
testglxqueryversion_LDADD = -lX11
testglxqueryversion_LDADD += libGLX.la
testglxqueryversion_LDADD += libOpenGL.la
testglxqueryversion_CFLAGS = $(TESTS_COMMON_CFLAGS)


check_PROGRAMS += testpatchentrypoints
testpatchentrypoints_SOURCES = \
	tests/testpatchentrypoints.c \
	tests/test_utils.c \
	src/util/trace.c

testpatchentrypoints_LDADD = -lX11 -ldl
testpatchentrypoints_LDADD += libGLX.la
testpatchentrypoints_LDADD += libOpenGL.la
testpatchentrypoints_CFLAGS = $(TESTS_COMMON_CFLAGS)

endif # ENABLE_GLX


# Start of EGL-specific tests.

TESTS_EGL =
TESTS_EGL += tests/testegldisplay.sh
TESTS_EGL += tests/testegldevice.sh
TESTS_EGL += tests/testeglgetprocaddress.sh
TESTS_EGL += tests/testeglmakecurrent.sh
TESTS_EGL += tests/testeglerror.sh
TESTS_EGL += tests/testegldebug.sh

if ENABLE_EGL

TESTS += $(TESTS_EGL)

check_PROGRAMS += testegldisplay
testegldisplay_SOURCES = \
	tests/testegldisplay.c \
	tests/egl_test_utils.c
testegldisplay_LDADD = libEGL.la
testegldisplay_CFLAGS = $(TESTS_COMMON_CFLAGS)

check_PROGRAMS += testegldevice
testegldevice_SOURCES = \
	tests/testegldevice.c \
	tests/egl_test_utils.c
testegldevice_LDADD = libEGL.la
testegldevice_CFLAGS = $(TESTS_COMMON_CFLAGS)

check_PROGRAMS += testeglgetprocaddress
testeglgetprocaddress_SOURCES = \
	tests/testeglgetprocaddress.c \
	tests/egl_test_utils.c
testeglgetprocaddress_LDADD = libEGL.la
testeglgetprocaddress_CFLAGS = $(TESTS_COMMON_CFLAGS)

check_PROGRAMS += testeglmakecurrent
testeglmakecurrent_SOURCES = \
	tests/testeglmakecurrent.c \
	tests/egl_test_utils.c
testeglmakecurrent_LDADD = libEGL.la
testeglmakecurrent_LDADD += libOpenGL.la
testeglmakecurrent_CFLAGS = $(TESTS_COMMON_CFLAGS)

check_PROGRAMS += testeglerror
testeglerror_SOURCES = \
	tests/testeglerror.c \
	tests/egl_test_utils.c
testeglerror_LDADD = libEGL.la
testeglerror_LDADD += libOpenGL.la
testeglerror_CFLAGS = $(TESTS_COMMON_CFLAGS)


check_PROGRAMS += testegldebug
testegldebug_SOURCES = \
	tests/testegldebug.c \
	tests/egl_test_utils.c
testegldebug_LDADD = libEGL.la
testegldebug_CFLAGS = $(TESTS_COMMON_CFLAGS)

endif # ENABLE_EGL

EXTRA_DIST += $(TESTS_GLX) $(TESTS_EGL)
