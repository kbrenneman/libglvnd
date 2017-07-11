# Used by OpenGL vendor-neutral dispatch library

include src/GLdispatch/vnd-glapi/entry_files.mk

noinst_HEADERS += \
	src/GLdispatch/vnd-glapi/glapi.h \
	src/GLdispatch/vnd-glapi/entry_common.h \
	src/GLdispatch/vnd-glapi/entry.h \
	src/GLdispatch/vnd-glapi/mapi_tmp.h \
	src/GLdispatch/vnd-glapi/stub.h \
	src/GLdispatch/vnd-glapi/table.h \
	src/GLdispatch/vnd-glapi/u_compiler.h \
	src/GLdispatch/vnd-glapi/u_current.h \
	src/GLdispatch/vnd-glapi/u_execmem.h \
	src/GLdispatch/vnd-glapi/u_macros.h

EXTRA_LTLIBRARIES += libglapi.la

libglapi_la_SOURCES = \
	$(MAPI_GLDISPATCH_ENTRY_FILES) \
	src/GLdispatch/vnd-glapi/mapi_glapi.c \
	src/GLdispatch/vnd-glapi/stub.c \
	src/GLdispatch/vnd-glapi/table.c \
	src/GLdispatch/vnd-glapi/u_execmem.c \
	src/util/utils_misc.c

# Select the appropriate file for looking up the current dispatch table.
if GLDISPATCH_USE_TLS
libglapi_la_SOURCES += src/GLdispatch/vnd-glapi/u_current_tls.c
else
libglapi_la_SOURCES += src/GLdispatch/vnd-glapi/u_current_tsd.c
endif
libglapi_la_LDFLAGS = $(LINKER_FLAG_NO_UNDEFINED)

glapi_mapi_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, gldispatch)

BUILT_SOURCES += glapi_mapi_tmp.h

GLAPI_COMMON_CFLAGS = \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/util \
	-I$(top_srcdir)/src/GLdispatch/vnd-glapi \
	-I$(top_builddir)//src/GLdispatch/vnd-glapi \
	-I$(top_srcdir)/$(MAPI_MESA_PREFIX)

libglapi_la_CFLAGS = \
	$(GLAPI_COMMON_CFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/glapi_mapi_tmp.h\"


ENTRYPOINT_SOURCES = \
	$(MAPI_GLDISPATCH_ENTRY_FILES) \
	src/GLdispatch/vnd-glapi/stub.c

ENTRYPOINT_CFLAGS = \
	$(GLAPI_COMMON_CFLAGS) \
	-DSTATIC_DISPATCH_ONLY

EXTRA_LTLIBRARIES += libglapi_gl.la
libglapi_gl_la_SOURCES = $(ENTRYPOINT_SOURCES)
libglapi_gl_la_CFLAGS = $(ENTRYPOINT_CFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_gl_tmp.h\"
noinst_HEADERS += g_glapi_mapi_gl_tmp.h
BUILT_SOURCES += g_glapi_mapi_gl_tmp.h
g_glapi_mapi_gl_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, gl)

EXTRA_LTLIBRARIES += libglapi_opengl.la
libglapi_opengl_la_SOURCES = $(ENTRYPOINT_SOURCES)
libglapi_opengl_la_CFLAGS = $(ENTRYPOINT_CFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_opengl_tmp.h\"
noinst_HEADERS += g_glapi_mapi_opengl_tmp.h
BUILT_SOURCES += g_glapi_mapi_opengl_tmp.h
g_glapi_mapi_opengl_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, opengl)

EXTRA_LTLIBRARIES += libglapi_glesv1.la
libglapi_glesv1_la_SOURCES = $(ENTRYPOINT_SOURCES)
libglapi_glesv1_la_CFLAGS = $(ENTRYPOINT_CFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_glesv1_tmp.h\"
noinst_HEADERS += g_glapi_mapi_glesv1_tmp.h
BUILT_SOURCES += g_glapi_mapi_glesv1_tmp.h
g_glapi_mapi_glesv1_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, glesv1)

EXTRA_LTLIBRARIES += libglapi_glesv2.la
libglapi_glesv2_la_SOURCES = $(ENTRYPOINT_SOURCES)
libglapi_glesv2_la_CFLAGS = $(ENTRYPOINT_CFLAGS) \
	-DMAPI_ABI_HEADER=\"$(builddir)/g_glapi_mapi_glesv2_tmp.h\"
noinst_HEADERS += g_glapi_mapi_glesv2_tmp.h
BUILT_SOURCES += g_glapi_mapi_glesv2_tmp.h
g_glapi_mapi_glesv2_tmp.h : $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi, glesv2)

