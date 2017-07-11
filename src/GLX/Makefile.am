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

noinst_HEADERS += \
	src/GLX/libglxabipriv.h \
	src/GLX/libglxcurrent.h \
	src/GLX/libglxmapping.h \
	src/GLX/libglxgl.h \
	src/GLX/libglxthread.h \
	src/GLX/libglxproto.h

lib_LTLIBRARIES += libGLX.la

# Warning settings
# Include paths
libGLX_la_CFLAGS =  -I$(top_srcdir)/include
libGLX_la_CFLAGS += -I$(top_srcdir)/src/util
libGLX_la_CFLAGS += -I$(top_srcdir)/src/util/uthash/src
libGLX_la_CFLAGS += -I$(top_srcdir)/src/GLdispatch
libGLX_la_CFLAGS += $(GLPROTO_CFLAGS)

# Required library flags
libGLX_la_CFLAGS += $(PTHREAD_CFLAGS)

# Required libraries
libGLX_la_LIBADD = -ldl
libGLX_la_LIBADD += $(X11_LIBS)
libGLX_la_LIBADD += $(XEXT_LIBS)
libGLX_la_LIBADD += libGLdispatch.la

libGLX_la_LDFLAGS = -shared -Wl,-Bsymbolic -version-info 0 $(LINKER_FLAG_NO_UNDEFINED)

libGLX_la_SOURCES = \
	src/GLX/libglx.c \
	src/GLX/libglxmapping.c \
	src/GLX/libglxproto.c \
	src/util/trace.c \
	src/util/glvnd_pthread.c \
	src/util/glvnd_genentry.c \
	src/util/utils_misc.c \
	src/util/app_error_check.c \
	src/util/winsys_dispatch.c

