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

#include "u_macros.h"
#include "glapi.h"
#include "glvnd/GLdispatchABI.h"


// NOTE: These must be powers of two:
#define PPC64LE_ENTRY_SIZE 256
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

#define STUB_ASM_CODE(slot)                                     \
            "1000:\n\t"                                         \
            "  addis  2, 12, .TOC.-1000b@ha\n\t"                \
            "  addi   2, 2, .TOC.-1000b@l\n\t"                  \
            "  addis  11, 2, _glapi_Current@got@ha\n\t"         \
            "  ld     11, _glapi_Current@got@l(11)\n\t"         \
            "  ld     11, 0(11)\n\t"                            \
            "  cmpldi 11, 0\n\t"                                \
            "  beq    2000f\n"                                  \
            "1050:\n\t"                                         \
            "  ld     12, " slot "*8(11)\n\t"                   \
            "  mtctr  12\n\t"                                   \
            "  bctr\n"                                          \
            "2000:\n\t"                                         \
            "  mflr   0\n\t"                                    \
            "  std    0, 16(1)\n\t"                             \
            "  std    2, 40(1)\n\t"                             \
            "  stdu   1, -144(1)\n\t"                           \
            "  std    3, 56(1)\n\t"                             \
            "  stq    4, 64(1)\n\t"                             \
            "  stq    6, 80(1)\n\t"                             \
            "  stq    8, 96(1)\n\t"                             \
            "  std    10, 112(1)\n\t"                           \
            "  std    12, 128(1)\n\t"                           \
            "  addis  12, 2, _glapi_get_current@got@ha\n\t"     \
            "  ld     12, _glapi_get_current@got@l(12)\n\t"     \
            "  mtctr  12\n\t"                                   \
            "  bctrl\n\t"                                       \
            "  ld     2, 144+40(1)\n\t"                         \
            "  mr     11, 3\n\t"                                \
            "  ld     3, 56(1)\n\t"                             \
            "  lq     4, 64(1)\n\t"                             \
            "  lq     6, 80(1)\n\t"                             \
            "  lq     8, 96(1)\n\t"                             \
            "  ld     10, 112(1)\n\t"                           \
            "  ld     12, 128(1)\n\t"                           \
            "  addi   1, 1, 144\n\t"                            \
            "  ld     0, 16(1)\n\t"                             \
            "  mtlr   0\n\t"                                    \
            "  b      1050b\n"
    // Conceptually, this is:
    // {
    //     void **dispatchTable = _glapi_Current[GLAPI_CURRENT_DISPATCH];
    //     if (dispatchTable == NULL) {
    //         dispatchTable = _glapi_get_current();
    //     }
    //     jump_to_address(dispatchTable[slot]);
    // }
    //
    // Note that _glapi_Current is a simple global variable.
    // See the x86 or x86-64 TSD code for examples.

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"


__asm__(".balign " U_STRINGIFY(PPC64LE_PAGE_ALIGN) "\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:");
__asm__(".text\n");

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
    0xA6, 0x02, 0x08, 0x7C,    // <ENTRY+000>:    mflr   0
    0x10, 0x00, 0x01, 0xF8,    // <ENTRY+004>:    std    0, 16(1)
    0x80, 0x00, 0x6C, 0xE9,    // <ENTRY+008>:    ld     11, 9000f-1000b+0(12)
    0x00, 0x00, 0x6B, 0xE9,    // <ENTRY+012>:    ld     11, 0(11)
    0x00, 0x00, 0x2B, 0x28,    // <ENTRY+016>:    cmpldi 11, 0
    0x14, 0x00, 0x82, 0x41,    // <ENTRY+020>:    beq    2000f
    // 1050:
    0x90, 0x00, 0x0C, 0xE8,    // <ENTRY+024>:    ld     0, 9000f-1000b+16(12)
    0x2A, 0x00, 0x8B, 0x7D,    // <ENTRY+028>:    ldx    12, 11, 0
    0xA6, 0x03, 0x89, 0x7D,    // <ENTRY+032>:    mtctr  12
    0x20, 0x04, 0x80, 0x4E,    // <ENTRY+036>:    bctr
    // 2000:
    0x28, 0x00, 0x41, 0xF8,    // <ENTRY+040>:    std    2, 40(1)
    0x71, 0xFF, 0x21, 0xF8,    // <ENTRY+044>:    stdu   1, -144(1)
    0x38, 0x00, 0x61, 0xF8,    // <ENTRY+048>:    std    3, 56(1)
    0x42, 0x00, 0x81, 0xF8,    // <ENTRY+052>:    stq    4, 64(1)
    0x52, 0x00, 0xC1, 0xF8,    // <ENTRY+056>:    stq    6, 80(1)
    0x62, 0x00, 0x01, 0xF9,    // <ENTRY+060>:    stq    8, 96(1)
    0x70, 0x00, 0x41, 0xF9,    // <ENTRY+064>:    std    10, 112(1)
    0x80, 0x00, 0x81, 0xF9,    // <ENTRY+068>:    std    12, 128(1)
    0x88, 0x00, 0x8C, 0xE9,    // <ENTRY+072>:    ld     12, 9000f-1000b+8(12)
    0xA6, 0x03, 0x89, 0x7D,    // <ENTRY+076>:    mtctr  12
    0x21, 0x04, 0x80, 0x4E,    // <ENTRY+080>:    bctrl
    0x70, 0x00, 0x41, 0xE9,    // <ENTRY+084>:    ld     10, 112(1)
    0x78, 0x1B, 0x6B, 0x7C,    // <ENTRY+088>:    mr     11, 3
    0x38, 0x00, 0x61, 0xE8,    // <ENTRY+092>:    ld     3, 56(1)
    0x40, 0x00, 0x81, 0xE0,    // <ENTRY+096>:    lq     4, 64(1)
    0x50, 0x00, 0xC1, 0xE0,    // <ENTRY+100>:    lq     6, 80(1)
    0x60, 0x00, 0x01, 0xE1,    // <ENTRY+104>:    lq     8, 96(1)
    0x80, 0x00, 0x81, 0xE9,    // <ENTRY+108>:    ld     12, 128(1)
    0x90, 0x00, 0x21, 0x38,    // <ENTRY+112>:    addi   1, 1, 144
    0x10, 0x00, 0x01, 0xE8,    // <ENTRY+116>:    ld     0, 16(1)
    0xA6, 0x03, 0x08, 0x7C,    // <ENTRY+120>:    mtlr   0
    0x9C, 0xFF, 0xFF, 0x4B,    // <ENTRY+124>:    b      1050b
    // 9000:
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <ENTRY+128>:    .quad _glapi_Current
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // <ENTRY+136>:    .quad _glapi_get_current
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00  // <ENTRY+144>:    .quad <slot>*8
};

// These are the offsets in ENTRY_TEMPLATE of the values that we have to patch.
static const int TEMPLATE_OFFSET_CURRENT_TABLE = (sizeof(ENTRY_TEMPLATE) - 24);
static const int TEMPLATE_OFFSET_CURRENT_TABLE_GET = (sizeof(ENTRY_TEMPLATE) - 16);
static const int TEMPLATE_OFFSET_SLOT = (sizeof(ENTRY_TEMPLATE) - 8);

/*
 * These offsets are used in entry_generate_default_code
 * to patch the dispatch table index and any memory addresses in the generated
 * function.
 *
 * TEMPLATE_OFFSET_SLOT is the dispatch table index.
 *
 * TEMPLATE_OFFSET_CURRENT_TABLE is the address of the global _glapi_Current
 * variable.
 *
 * TEMPLATE_OFFSET_CURRENT_TABLE_GET is the address of the function
 * _glapi_get_current.
 */
void entry_generate_default_code(char *entry, int slot)
{
    char *writeEntry = u_execmem_get_writable(entry);
    memcpy(writeEntry, ENTRY_TEMPLATE, sizeof(ENTRY_TEMPLATE));

    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_SLOT)) = slot * sizeof(mapi_func);
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE)) = (uintptr_t) _glapi_Current;
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE_GET)) = (uintptr_t) _glapi_get_current;
}

