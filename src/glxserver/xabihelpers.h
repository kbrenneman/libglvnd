/*
 * Copyright (c) 2016, NVIDIA CORPORATION.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and/or associated documentation files (the
 * "Materials"), to deal in the Materials without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Materials, and to
 * permit persons to whom the Materials are furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * unaltered in all copies or substantial portions of the Materials.
 * Any additions, deletions, or changes to the original source files
 * must be clearly indicated in accompanying documentation.
 *
 * If only executable code is distributed, then the accompanying
 * documentation must state that "this software is based in part on the
 * work of the Khronos Group."
 *
 * THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.
 */

#ifndef XABIHELPERS_H
#define XABIHELPERS_H

#include <X11/Xlib.h>

#include <xorg-server.h>
#include <scrnintstr.h>
#include <dixstruct.h>
#include <extnsionst.h>

#if defined(__cplusplus)
extern "C" {
#endif

Bool xglvInitPrivateSpace(void);

void xglvLoadExtension(InitExtension initFunc, const char *name);

void *xglvGetScreenPrivate(ScreenPtr pScreen);
void xglvSetScreenPrivate(ScreenPtr pScreen, void *priv);

void *xglvGetClientPrivate(ClientPtr pClient);
void xglvSetClientPrivate(ClientPtr pClient, void *priv);

int xglvLookupResourceByType(pointer *result, XID id, RESTYPE rtype,
        ClientPtr client, Mask access_mode);

int xglvLookupResourceByClass(pointer *result, XID id, RESTYPE classes,
                                         ClientPtr client, Mask access_mode);

#if defined(__cplusplus)
}
#endif

#endif // XABIHELPERS_H
