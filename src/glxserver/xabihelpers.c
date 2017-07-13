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

#include "xabihelpers.h"

#include <xorgVersion.h>
#include <xf86.h>

#define XGLV_ABI_HAS_DIX_REGISTER_PRIVATE_KEY    (GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) >= 8)
#define XGLV_ABI_HAS_DEV_PRIVATE_REWORK          (GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) >= 4)
#define XGLV_ABI_HAS_DIX_LOOKUP_RES_BY           (GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) >= 6)
#define XGLV_ABI_EXTENSION_MODULE_HAS_SETUP_FUNC_AND_INIT_DEPS \
                                                 (GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) <= 12)
#define XGLV_ABI_HAS_LOAD_EXTENSION_LIST         (GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) >= 17)
#define XGLV_ABI_SWAP_MACROS_HAVE_N_ARG          (GET_ABI_MAJOR(ABI_VIDEODRV_VERSION) <= 11)

void xglvLoadExtension(InitExtension initFunc, const char *name)
{
    ExtensionModule module = { initFunc, name };

#if XGLV_ABI_HAS_LOAD_EXTENSION_LIST
    LoadExtensionList(&module, 1, False);
#else
    LoadExtension(&module, False);
#endif
}

#if XGLV_ABI_HAS_DIX_REGISTER_PRIVATE_KEY
// ABI >= 8

static DevPrivateKeyRec glvXGLVScreenPrivKey;
static DevPrivateKeyRec glvXGLVClientPrivKey;

Bool xglvInitPrivateSpace(void)
{
    if (!dixRegisterPrivateKey(&glvXGLVScreenPrivKey, PRIVATE_SCREEN, 0)) {
        return False;
    }
    if (!dixRegisterPrivateKey(&glvXGLVClientPrivKey, PRIVATE_CLIENT, 0)) {
        return False;
    }
    return True;
}

void *xglvGetScreenPrivate(ScreenPtr pScreen)
{
    return dixLookupPrivate(&pScreen->devPrivates, &glvXGLVScreenPrivKey);
}

void xglvSetScreenPrivate(ScreenPtr pScreen, void *priv)
{
    dixSetPrivate(&pScreen->devPrivates, &glvXGLVScreenPrivKey, priv);
}

void *xglvGetClientPrivate(ClientPtr pClient)
{
    return dixLookupPrivate(&pClient->devPrivates, &glvXGLVClientPrivKey);
}

void xglvSetClientPrivate(ClientPtr pClient, void *priv)
{
    dixSetPrivate(&pClient->devPrivates, &glvXGLVClientPrivKey, priv);
}

#elif XGLV_ABI_HAS_DEV_PRIVATE_REWORK // XGLV_ABI_HAS_DIX_REGISTER_PRIVATE_KEY
// ABI 4 - 7
// In ABI 5, DevPrivateKey is int* and needs to point to a unique int.
// In ABI 4, DevPrivateKey is void* and just needs to be unique.
// We just use the ABI 5 behavior for both for consistency.
static int glvXGLVScreenPrivKey;
static int glvXGLVClientPrivKey;

Bool xglvInitPrivateSpace(void)
{
    if (!dixRequestPrivate(&glvXGLVScreenPrivKey, 0)) {
        return False;
    }
    if (!dixRequestPrivate(&glvXGLVClientPrivKey, 0)) {
        return False;
    }
    return True;
}

void *xglvGetScreenPrivate(ScreenPtr pScreen)
{
    return dixLookupPrivate(&pScreen->devPrivates, &glvXGLVScreenPrivKey);
}

void xglvSetScreenPrivate(ScreenPtr pScreen, void *priv)
{
    dixSetPrivate(&pScreen->devPrivates, &glvXGLVScreenPrivKey, priv);
}

void *xglvGetClientPrivate(ClientPtr pClient)
{
    return dixLookupPrivate(&pClient->devPrivates, &glvXGLVClientPrivKey);
}

void xglvSetClientPrivate(ClientPtr pClient, void *priv)
{
    dixSetPrivate(&pClient->devPrivates, &glvXGLVClientPrivKey, priv);
}

#define NV_REQUEST_GLOBAL_PRIVATE_SPACE(type_enum, type, key, size) \
    dixRequestPrivate(&nv##key##Key, (size))

#else
// ABI <= 3
static int glvXGLVScreenPrivKey = -1;
static int glvXGLVClientPrivKey = -1;

Bool xglvInitPrivateSpace(void)
{
    glvXGLVScreenPrivKey = AllocateScreenPrivateIndex();
    if (glvXGLVScreenPrivKey < 0) {
        return False;
    }
    glvXGLVClientPrivKey = AllocateClientPrivateIndex();
    if (glvXGLVClientPrivKey < 0) {
        return False;
    }

    return True;
}

void *xglvGetScreenPrivate(ScreenPtr pScreen)
{
    return (pScreen->devPrivates[glvXGLVScreenPrivKey].ptr);
}

void xglvSetScreenPrivate(ScreenPtr pScreen, void *priv)
{
    pScreen->devPrivates[glvXGLVScreenPrivKey].ptr = priv;
}

void *xglvGetClientPrivate(ClientPtr pClient)
{
    return (pClient->devPrivates[glvXGLVClientPrivKey].ptr);
}

void xglvSetClientPrivate(ClientPtr pClient, void *priv)
{
    pClient->devPrivates[glvXGLVClientPrivKey].ptr = priv;
}

#endif

int xglvLookupResourceByType(pointer *result, XID id, RESTYPE rtype,
        ClientPtr client, Mask access_mode)
{
#if XGLV_ABI_HAS_DIX_LOOKUP_RES_BY
    return dixLookupResourceByType(result, id, rtype, client, access_mode);
#elif XGLV_ABI_HAS_DEV_PRIVATE_REWORK
    return dixLookupResource(result, id, rtype, client, access_mode);
#else
    *result = SecurityLookupIDByType(client, id, rtype, access_mode);
    return (*result ? Success : BadValue);
#endif
}

int xglvLookupResourceByClass(pointer *result, XID id, RESTYPE classes,
                                         ClientPtr client, Mask access_mode)
{
#if XGLV_ABI_HAS_DIX_LOOKUP_RES_BY
    return dixLookupResourceByClass(result, id, classes, client, access_mode);
#elif XGLV_ABI_HAS_DEV_PRIVATE_REWORK
    return dixLookupResource(result, id, classes, client, access_mode);
#else
    *result = SecurityLookupIDByClass(client, id, classes, access_mode);
    return *retval ? Success : failureCode;
#endif
}
