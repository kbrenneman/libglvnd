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

#include "glxserver.h"

#include <pixmapstr.h>

#include "glxservervendor.h"
#include "xabihelpers.h"

// The resource type used to keep track of the vendor library for XID's.
static RESTYPE idResource;

static int idResorceDeleteCallback(void *value, XID id);

Bool __glXMappingInit(void)
{
    int i;

    for (i=0; i<screenInfo.numScreens; i++) {
        if (__glXGetScreen(screenInfo.screens[i]) == NULL) {
            __glXMappingReset();
            return False;
        }
    }

    idResource = CreateNewResourceType(idResorceDeleteCallback, "GLXServerIDRes");
    if (idResource == RT_NONE)
    {
        __glXMappingReset();
        return False;
    }
    return True;
}

void __glXMappingReset(void)
{
    int i;

    for (i=0; i<screenInfo.numScreens; i++) {
        __GLXScreenPriv *priv = (__GLXScreenPriv *) xglvGetScreenPrivate(screenInfo.screens[i]);
        if (priv != NULL) {
            xglvSetScreenPrivate(screenInfo.screens[i], NULL);
            free(priv);
        }
    }
}

__GLXClientPriv *__glXGetClientData(ClientPtr client)
{
    __GLXClientPriv *cl = (__GLXClientPriv *) xglvGetClientPrivate(client);
    if (cl == NULL) {
        cl = calloc(1, sizeof(__GLXClientPriv));
        if (cl != NULL) {
            cl->client = client;
            xglvSetClientPrivate(client, cl);
        }
    }
    return cl;
}

void __glXFreeClientData(ClientPtr client)
{
    __GLXClientPriv *cl = (__GLXClientPriv *) xglvGetClientPrivate(client);
    if (cl != NULL) {
        xglvSetClientPrivate(client, NULL);
        free(cl->contextTags);
        free(cl);
    }
}

int idResorceDeleteCallback(void *value, XID id)
{
    return 0;
}

static __GLXServerVendor *LookupXIDMapResource(XID id)
{
    void *ptr = NULL;
    int rv;

    rv = xglvLookupResourceByType(&ptr, id,
            idResource, NULL, DixReadAccess);
    if (rv == Success) {
        return (__GLXServerVendor *) ptr;
    } else {
        return NULL;
    }
}

__GLXServerVendor *__glXGetXIDMap(XID id)
{
    __GLXServerVendor *vendor = LookupXIDMapResource(id);

    if (vendor == NULL) {
        // If we haven't seen this XID before, then it may be a drawable that
        // wasn't created through GLX, like a regular X window or pixmap. Try
        // to look up a matching drawable to find a screen number for it.
        void *ptr = NULL;
        int rv = xglvLookupResourceByType(&ptr, id,
                RC_DRAWABLE, NULL, DixGetAttrAccess);
        if (rv == Success && ptr != NULL) {
            DrawablePtr draw = (DrawablePtr) ptr;
            __GLXScreenPriv *screenPriv = __glXGetScreen(draw->pScreen);
            if (screenPriv != NULL) {
                vendor = screenPriv->vendor;
            }
        }
    }
    return vendor;
}

Bool __glXAddXIDMap(XID id, __GLXServerVendor *vendor)
{
    if (id == 0 || vendor == NULL) {
        return False;
    }
    if (LookupXIDMapResource(id) != NULL) {
        return False;
    }
    return AddResource(id, idResource, vendor);
}

void __glXRemoveXIDMap(XID id)
{
    FreeResourceByType(id, idResource, False);
}

__GLXContextTagInfo *__glXAllocContextTag(ClientPtr client, __GLXServerVendor *vendor)
{
    __GLXClientPriv *cl;
    unsigned int index;

    if (vendor == NULL) {
        return NULL;
    }

    cl = __glXGetClientData(client);
    if (cl == NULL) {
        return NULL;
    }

    // Look for a free tag index.
    for (index=0; index<cl->contextTagCount; index++) {
        if (cl->contextTags[index].vendor == NULL) {
            break;
        }
    }
    if (index >= cl->contextTagCount) {
        // We didn't find a free entry, so grow the array.
        __GLXContextTagInfo *newTags;
        unsigned int newSize = cl->contextTagCount * 2;
        if (newSize == 0) {
            // TODO: What's a good starting size for this?
            newSize = 16;
        }

        newTags = (__GLXContextTagInfo *)
            realloc(cl->contextTags, newSize * sizeof(__GLXContextTagInfo));
        if (newTags == NULL) {
            return NULL;
        }

        memset(&newTags[cl->contextTagCount], 0,
                (newSize - cl->contextTagCount) * sizeof(__GLXContextTagInfo));

        index = cl->contextTagCount;
        cl->contextTags = newTags;
        cl->contextTagCount = newSize;
    }

    assert(index >= 0);
    assert(index < cl->contextTagCount);
    memset(&cl->contextTags[index], 0, sizeof(__GLXContextTagInfo));
    cl->contextTags[index].tag = (GLXContextTag) (index + 1);
    cl->contextTags[index].client = client;
    cl->contextTags[index].vendor = vendor;
    return &cl->contextTags[index];
}

__GLXContextTagInfo *__glXLookupContextTag(ClientPtr client, GLXContextTag tag)
{
    __GLXClientPriv *cl = __glXGetClientData(client);
    if (cl == NULL) {
        return NULL;
    }

    if (tag > 0 && (tag - 1) < cl->contextTagCount) {
        if (cl->contextTags[tag - 1].vendor != NULL) {
            assert(cl->contextTags[tag - 1].client == client);
            return &cl->contextTags[tag - 1];
        }
    }
    return NULL;
}

void __glXFreeContextTag(__GLXContextTagInfo *tagInfo)
{
    if (tagInfo != NULL) {
        tagInfo->vendor = NULL;
        tagInfo->vendor = NULL;
        tagInfo->data = NULL;
        tagInfo->context = None;
        tagInfo->drawable = None;
        tagInfo->readdrawable = None;
    }
}

__GLXScreenPriv *__glXGetScreen(ScreenPtr pScreen)
{
    if (pScreen != NULL) {
        __GLXScreenPriv *priv = (__GLXScreenPriv *) xglvGetScreenPrivate(pScreen);
        if (priv == NULL) {
            priv = (__GLXScreenPriv *) calloc(1, sizeof(__GLXScreenPriv));
            if (priv == NULL) {
                return NULL;
            }

            xglvSetScreenPrivate(pScreen, priv);
        }
        return priv;
    } else {
        return NULL;
    }
}

Bool __glXSetScreenVendor(ScreenPtr screen, __GLXServerVendor *vendor)
{
    __GLXScreenPriv *priv;

    if (vendor == NULL) {
        return False;
    }
    if (!vendor->initialized) {
        return False;
    }

    priv = __glXGetScreen(screen);
    if (priv == NULL) {
        return False;
    }

    priv->vendor = vendor;
    return True;
}

__GLXServerVendor *__glXGetVendorForScreen(ClientPtr client, ScreenPtr screen)
{
    __GLXScreenPriv *priv = __glXGetScreen(screen);
    if (priv != NULL) {
        return priv->vendor;
    } else {
        return NULL;
    }
}
