# This fragment lists the files used for the dispatch stub implementation in
# libGLdispatch, libOpenGL, and libGL.
# The conditionals here are all set from the configure script.

if GLDISPATCH_TYPE_X86_TLS
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_x86_tls.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_simple_asm.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_common.c
endif

if GLDISPATCH_TYPE_X86_TSD
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_x86_tsd.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_simple_asm.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_common.c
endif

if GLDISPATCH_TYPE_X86_64_TLS
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_x86_64_tls.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_simple_asm.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_common.c
endif

if GLDISPATCH_TYPE_X86_64_TSD
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_x86_64_tsd.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_simple_asm.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_common.c
endif


if GLDISPATCH_TYPE_ARMV7_TSD
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_armv7_tsd.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_common.c
endif

if GLDISPATCH_TYPE_AARCH64_TSD
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_aarch64_tsd.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_simple_asm.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_common.c
endif

if GLDISPATCH_TYPE_PPC64LE_TSD
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_ppc64le_tsd.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_simple_asm.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_common.c
endif

if GLDISPATCH_TYPE_PPC64LE_TLS
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_ppc64le_tls.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_simple_asm.c
MAPI_GLDISPATCH_ENTRY_FILES += src/GLdispatch/vnd-glapi/entry_common.c
endif

if GLDISPATCH_TYPE_PURE_C
MAPI_GLDISPATCH_ENTRY_FILES = src/GLdispatch/vnd-glapi/entry_pure_c.c
endif

# vim:filetype=automake
