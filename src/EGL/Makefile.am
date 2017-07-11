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
	src/EGL/egldispatchstubs.h \
	src/EGL/libeglabipriv.h \
	src/EGL/libeglcurrent.h \
	src/EGL/libeglmapping.h \
	src/EGL/libeglvendor.h \
	src/EGL/libeglerror.h

lib_LTLIBRARIES += libEGL.la

# Warning settings
# Include paths
libEGL_la_CFLAGS  = -I$(top_srcdir)/include
libEGL_la_CFLAGS += -I$(top_srcdir)/src/util
libEGL_la_CFLAGS += -I$(top_srcdir)/src/util/uthash/src
libEGL_la_CFLAGS += -I$(top_srcdir)/src/GLdispatch
libEGL_la_CFLAGS += -I$(top_srcdir)/src/EGL

libEGL_la_CFLAGS += \
	-DDEFAULT_EGL_VENDOR_CONFIG_DIRS=\"@sysconfdir@/glvnd/egl_vendor.d:@datadir@/glvnd/egl_vendor.d\"

# Required library flags
libEGL_la_CFLAGS += $(PTHREAD_CFLAGS)

# Required libraries
libEGL_la_LIBADD = -ldl
libEGL_la_LIBADD += -lm
libEGL_la_LIBADD += libGLdispatch.la
libEGL_la_LIBADD += libcJSON.la
libEGL_la_LIBADD += libEGL_dispatch_stubs.la

libEGL_la_LDFLAGS = -shared -Wl,-Bsymbolic -version-info 1 $(LINKER_FLAG_NO_UNDEFINED)

libEGL_la_SOURCES = \
	src/EGL/libegl.c \
	src/EGL/libeglcurrent.c \
	src/EGL/libeglmapping.c \
	src/EGL/libeglvendor.c \
	src/EGL/libeglerror.c \
	src/util/trace.c \
	src/util/glvnd_pthread.c \
	src/util/utils_misc.c \
	src/util/winsys_dispatch.c

# The generated EGL dispatch stubs are build independantly of the rest of the
# EGL library. Use a convenience library here to make it easier to move them
# to another directory.

EXTRA_LTLIBRARIES += libEGL_dispatch_stubs.la
libEGL_dispatch_stubs_la_CFLAGS = -I$(top_srcdir)/src/EGL
libEGL_dispatch_stubs_la_CFLAGS += -I$(top_srcdir)/include
libEGL_dispatch_stubs_la_SOURCES = \
	src/EGL/egldispatchstubs.c \
	g_egldispatchstubs.c

BUILT_SOURCES += g_egldispatchstubs.c g_egldispatchstubs.h

EGL_GENERATE_DISPATCH_SCRIPT = $(top_srcdir)/src/generate/gen_egl_dispatch.py
EGL_GENERATE_LIST_FILES = \
	$(top_srcdir)/src/generate/eglFunctionList.py \
	$(top_srcdir)/src/generate/xml/egl.xml

EGL_GENERATE_DEPS = \
	$(top_srcdir)/src/generate/genCommon.py \
	$(EGL_GENERATE_DISPATCH_SCRIPT) \
	$(EGL_GENERATE_LIST_FILES)

g_egldispatchstubs.c : $(EGL_GENERATE_DEPS)
	$(VM_V_GEN)$(PYTHON2) $(EGL_GENERATE_DISPATCH_SCRIPT) source $(EGL_GENERATE_LIST_FILES) > $@

g_egldispatchstubs.h : $(EGL_GENERATE_DEPS)
	$(VM_V_GEN)$(PYTHON2) $(EGL_GENERATE_DISPATCH_SCRIPT) header $(EGL_GENERATE_LIST_FILES) > $@

EXTRA_LTLIBRARIES += libcJSON.la
libcJSON_la_SOURCES = src/util/cJSON.c
libcJSON_la_CFLAGS = -Wno-misleading-indentation
