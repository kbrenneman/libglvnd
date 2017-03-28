/*
 * Copyright (c) 2015, NVIDIA CORPORATION.
 *
 * Copyright (C) 2010 LunarG Inc.
 * Copyright (c) 2015, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include "entry.h"
#include "entry_common.h"

#include <assert.h>
#include <stdint.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

#include "utils_misc.h"
#include "u_macros.h"
#include "glapi.h"
#include "glvnd/GLdispatchABI.h"


// NOTE: These must be powers of two:
#define PPC64LE_ENTRY_SIZE 64
#define PPC64LE_PAGE_ALIGN 65536
#if ((PPC64LE_ENTRY_SIZE & (PPC64LE_ENTRY_SIZE - 1)) != 0)
#error PPC64LE_ENTRY_SIZE must be a power of two!
#endif
#if ((PPC64LE_PAGE_ALIGN & (PPC64LE_PAGE_ALIGN - 1)) != 0)
#error PPC64LE_PAGE_ALIGN must be a power of two!
#endif

__asm__(".section wtext,\"ax\",@progbits\n");
__asm__(".balign " U_STRINGIFY(PPC64LE_PAGE_ALIGN) "\n"
       ".globl public_entry_start\n"
       ".hidden public_entry_start\n"
        "public_entry_start:");

#define STUB_ASM_ENTRY(func)        \
   ".globl " func "\n"              \
   ".type " func ", @function\n"    \
   ".balign " U_STRINGIFY(PPC64LE_ENTRY_SIZE) "\n"                   \
   func ":"

#define STUB_ASM_CODE(slot)                                         \
            "1000:\n\t"                                                 \
            "  addis  2, 12, .TOC.-1000b@ha\n\t"                        \
            "  addi   2, 2, .TOC.-1000b@l\n\t"                          \
            "  addis  11, 2, _glapi_tls_Current@got@tprel@ha\n\t"       \
            "  ld     11, _glapi_tls_Current@got@tprel@l(11)\n\t"       \
            "  add    11, 11,_glapi_tls_Current@tls\n\t"                \
            "  ld     11, 0(11)\n\t"                                    \
            "  ld     12, " slot "*8(11)\n\t"                           \
            "  mtctr  12\n\t"                                           \
            "  bctr\n"                                                  \
    // Conceptually, this is:
    // {
    //     void **dispatchTable = _glapi_tls_Current;
    //     jump_to_address(dispatchTable[slot];
    // }
    //
    // Note that _glapi_tls_Current is a global variable declared with
    // __thread.

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"


__asm__(".balign " U_STRINGIFY(PPC64LE_PAGE_ALIGN) "\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:");

__asm__(".text\n");

__asm__("ppc64le_current_tls:\n\t"
        "  addis  3, 2, _glapi_tls_Current@got@tprel@ha\n\t"
        "  ld     3, _glapi_tls_Current@got@tprel@l(3)\n\t"
        "  blr\n"
        );

extern uint64_t ppc64le_current_tls();

const int entry_type = __GLDISPATCH_STUB_PPC64LE;
const int entry_stub_size = PPC64LE_ENTRY_SIZE;

static const uint8_t ENTRY_TEMPLATE[] =
{
    // This should be functionally the same code as would be generated from
    // the STUB_ASM_CODE macro, but defined as a buffer.
    // This is used to generate new dispatch stubs. libglvnd will copy this
    // data to the dispatch stub, and then it will patch the slot number and
    // any addresses that it needs to.
    // 1000:
    0xA6, 0x02, 0x08, 0x7C,    // <ENTRY+00>:   mflr   0
    0x10, 0x00, 0x01, 0xF8,    // <ENTRY+04>:   std    0, 16(1)
    0x28, 0x00, 0x6C, 0xE9,    // <ENTRY+08>:   ld     11, 9000f-1000b+0(12)
    0x14, 0x6A, 0x6B, 0x7D,    // <ENTRY+12>:   add    11, 11, 13
    0x00, 0x00, 0x6B, 0xE9,    // <ENTRY+16>:   ld     11, 0(11)
    0x30, 0x00, 0x0C, 0xE8,    // <ENTRY+20>:   ld     0, 9000f-1000b+8(12)
    0x2A, 0x00, 0x8B, 0x7D,    // <ENTRY+24>:   ldx    12, 11, 0
    0xA6, 0x03, 0x89, 0x7D,    // <ENTRY+28>:   mtctr  12
    0x20, 0x04, 0x80, 0x4E,    // <ENTRY+32>:   bctr
    0x00, 0x00, 0x00, 0x60,    // <ENTRY+36>:   nop
    // 9000:
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <ENTRY+40>:    .quad _glapi_Current
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // <ENTRY+48>:    .quad <slot>*8
};

/*
 * These are the offsets in ENTRY_TEMPLATE used in entry_generate_default_code
 * to patch the dispatch table index and the slot number in the generated
 * function.
 *
 * TEMPLATE_OFFSET_TLS_ADDR is the offset part of the _glapi_tls_Current
 *__thread variable,
 * TEMPLATE_OFFSET_SLOT is the dispatch table index.
 */

static const int TEMPLATE_OFFSET_TLS_ADDR = sizeof(ENTRY_TEMPLATE) - 16;
static const int TEMPLATE_OFFSET_SLOT = sizeof(ENTRY_TEMPLATE) - 8;

void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry = u_execmem_get_writable(entry);

    STATIC_ASSERT(PPC64LE_ENTRY_SIZE >= sizeof(ENTRY_TEMPLATE));

    assert(slot >= 0);

    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));

    *((uint64_t *) (writeEntry + TEMPLATE_OFFSET_TLS_ADDR)) = (uintptr_t) ppc64le_current_tls();
    *((uint64_t *) (writeEntry + TEMPLATE_OFFSET_SLOT)) = slot * sizeof(mapi_func);
}

