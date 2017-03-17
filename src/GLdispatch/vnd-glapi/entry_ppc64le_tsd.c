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


// TODO: Change this macro to be the size of the dispatch stubs.
#define PPC64LE_ENTRY_SIZE 128

__asm__(".section wtext,\"ax\",@progbits\n");
__asm__(".balign 4096\n"
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
            "  addis  11, 2, _glapi_Current@got@ha\n\t"         \
            "  ld     11, _glapi_Current@got@l(11)\n\t"         \
            "  std    3, 0(11)\n\t"                             \
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
    //         dispatchTable = _glapi_Current[GLAPI_CURRENT_DISPATCH] = _glapi_get_current();
    //     }
    //     jump_to_address(dispatchTable[slot]);
    // }
    //
    // Note that _glapi_Current is a simple global variable.
    // See the x86 or x86-64 TSD code for examples.

#define MAPI_TMP_STUB_ASM_GCC
#include "mapi_tmp.h"


__asm__(".balign 4096\n"
       ".globl public_entry_end\n"
       ".hidden public_entry_end\n"
        "public_entry_end:");
__asm__(".text\n");

const int entry_type = __GLDISPATCH_STUB_PPC64LE;
const int entry_stub_size = PPC64LE_ENTRY_SIZE;

static const uint32_t ENTRY_TEMPLATE[] =
{
    // This should be functionally the same code as would be generated from
    // the STUB_ASM_CODE macro, but defined as a buffer.
    // This is used to generate new dispatch stubs. libglvnd will copy this
    // data to the dispatch stub, and then it will patch the slot number and
    // any addresses that it needs to.
    // 1000:
    0xA602087C,     // <ENTRY+000>:    mflr   0
    0x100001F8,     // <ENTRY+004>:    std    0, 16(1)
    0x90006CE9,     // <ENTRY+008>:    ld     11, 9000f-1000b+0(12)
    0x00006BE9,     // <ENTRY+012>:    ld     11, 0(11)
    0x00002B28,     // <ENTRY+016>:    cmpldi 11, 0
    0x14008241,     // <ENTRY+020>:    beq    2000f
    // 1050:
    0xA0000CE8,     // <ENTRY+024>:    ld     0, 9000f-1000b+16(12)
    0x2A008B7D,     // <ENTRY+028>:    ldx    12, 11, 0
    0xA603897D,     // <ENTRY+032>:    mtctr  12
    0x2004804E,     // <ENTRY+036>:    bctr
    // 2000:
    0x280041F8,     // <ENTRY+040>:    std    2, 40(1)
    0x71FF21F8,     // <ENTRY+044>:    stdu   1, -144(1)
    0x380061F8,     // <ENTRY+048>:    std    3, 56(1)
    0x420081F8,     // <ENTRY+052>:    stq    4, 64(1)
    0x5200C1F8,     // <ENTRY+056>:    stq    6, 80(1)
    0x620001F9,     // <ENTRY+060>:    stq    8, 96(1)
    0x700041F9,     // <ENTRY+064>:    std    10, 112(1)
    0x800081F9,     // <ENTRY+068>:    std    12, 128(1)
    0x98008CE9,     // <ENTRY+072>:    ld     12, 9000f-1000b+8(12)
    0xA603897D,     // <ENTRY+076>:    mtctr  12
    0x2104804E,     // <ENTRY+080>:    bctrl
    0x700041E9,     // <ENTRY+084>:    ld     10, 112(1)
    0x800081E9,     // <ENTRY+088>:    ld     12, 128(1)
    0x90006CE9,     // <ENTRY+092>:    ld     11, 9000f-1000b+0(12)
    0x00006BF8,     // <ENTRY+096>:    std    3, 0(11)
    0x781B6B7C,     // <ENTRY+100>:    mr     11, 3
    0x380061E8,     // <ENTRY+104>:    ld     3, 56(1)
    0x400081E0,     // <ENTRY+108>:    lq     4, 64(1)
    0x5000C1E0,     // <ENTRY+112>:    lq     6, 80(1)
    0x600001E1,     // <ENTRY+116>:    lq     8, 96(1)
    0x800081E9,     // <ENTRY+120>:    ld     12, 128(1)
    0x90002138,     // <ENTRY+124>:    addi   1, 1, 144
    0x100001E8,     // <ENTRY+128>:    ld     0, 16(1)
    0xA603087C,     // <ENTRY+132>:    mtlr   0
    0x90FFFF4B,     // <ENTRY+136>:    b      1050b
    0x00000060,     // <ENTRY+140>:    nop
    // 9000:
    0x00000000, 0x00000000,     // <ENTRY+144>:   .quad dispatch
    0x00000000, 0x00000000,     // <ENTRY+152>:   .quad get_current
    0x00000000, 0x00000000      // <ENTRY+160>:   .quad <slot>*8

};

// These are the offsets in ENTRY_TEMPLATE of the values that we have to patch.
static const int TEMPLATE_OFFSET_CURRENT_TABLE = 144;
static const int TEMPLATE_OFFSET_CURRENT_TABLE_GET = 152;
static const int TEMPLATE_OFFSET_SLOT = 160;

/*
 * TODO: Fill in these offsets. These are used in entry_generate_default_code
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

    // TODO: Patch the dispatch table slot and the addresses of the
    // _glapi_Current variable and _glapi_get_current function.
    // Note that (entry) is the executable pointer, and (writeEntry) is the
    // writable pointer, which may or may not be the same. So, if the stub
    // needs to use IP-relative addresses, then calculate those addresses based
    // on (entry).
    *((uint32_t *) (writeEntry + TEMPLATE_OFFSET_SLOT)) = slot * sizeof(mapi_func);
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE)) = (uintptr_t) _glapi_Current;
    *((uintptr_t *) (writeEntry + TEMPLATE_OFFSET_CURRENT_TABLE_GET)) = (uintptr_t) _glapi_get_current;

    // TODO: Do any cache clears or anything else that is necessary on PPC64LE
    // to make self-modifying code work.
}

