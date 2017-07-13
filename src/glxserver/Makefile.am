# Copyright (c) 2016, NVIDIA CORPORATION.
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

noinst_HEADERS =
noinst_HEADERS += xabihelpers.h
noinst_HEADERS += glxservervendor.h
noinst_HEADERS += glxserver.h

glxserver_LTLIBRARIES = libglx.la
glxserverdir = $(libdir)/xorg/modules/extensions

libglx_la_CFLAGS = -I$(top_srcdir)/include
libglx_la_CFLAGS += -I$(top_srcdir)/src/util
libglx_la_CFLAGS += -I$(top_srcdir)/src/util/uthash/src
libglx_la_CFLAGS += $(XORG_CFLAGS)
libglx_la_CFLAGS += $(X11_CFLAGS)
libglx_la_CFLAGS += $(GLPROTO_CFLAGS)

# HACK: Only do this on 64-bit systems.
# Why didn't this break with the x11glvnd module?
libglx_la_CFLAGS += -D_XSERVER64

libglx_la_LDFLAGS = \
	-module -avoid-version

g_dispatch_stubs.c: generate_dispatch_stubs.py
	$(VM_V_GEN)$(PYTHON2) $(srcdir)/generate_dispatch_stubs.py > $@

BUILT_SOURCES = g_dispatch_stubs.c
CLEANFILES = $(BUILT_SOURCES)
EXTRA_DIST = generate_dispatch_stubs.py

libglx_la_SOURCES =
libglx_la_SOURCES += xabihelpers.c
libglx_la_SOURCES += glxservervendor.c
libglx_la_SOURCES += glxservermapping.c
libglx_la_SOURCES += glxcmds.c
libglx_la_SOURCES += glxext.c
libglx_la_SOURCES += g_dispatch_stubs.c

libglx_la_LIBADD =
libglx_la_LIBADD += ../util/libutils_misc.la

