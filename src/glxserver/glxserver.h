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

#ifndef GLXSERVER_H
#define GLXSERVER_H

#include <xorg-server.h>
#include "glvnd/glxserverabi.h"

#define GLXContextID CARD32
#define GLXDrawable CARD32

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct __GLXScreenPrivRec {
    __GLXServerVendor *vendor;
} __GLXScreenPriv;

typedef struct __GLXContextTagInfoRec {
    GLXContextTag tag;
    ClientPtr client;
    __GLXServerVendor *vendor;
    void *data;
    GLXContextID context;
    GLXDrawable drawable;
    GLXDrawable readdrawable;
} __GLXContextTagInfo;

typedef struct __GLXClientPrivRec {
    ClientPtr client;
    __GLXContextTagInfo *contextTags;
    unsigned int contextTagCount;
} __GLXClientPriv;

typedef struct {
    int opcode;
    __GLXServerDispatchProc proc;
} __GLXGeneratedDispatchFunc;

extern const __GLXserverExports __glXvendorExports;
extern int __glXerrorBase;

extern const __GLXGeneratedDispatchFunc GENERATED_DISPATCH_LIST[];

static inline CARD32 __glXCheckSwap(ClientPtr client, CARD32 value)
{
    if (client->swapped)
    {
        value = ((value & 0XFF000000) >> 24) | ((value & 0X00FF0000) >>  8)
            | ((value & 0X0000FF00) <<  8) | ((value & 0X000000FF) << 24);
    }
    return value;
}

// Defined in glxext.c.
const ExtensionEntry *__glXGetExtensionEntry(void);

Bool __glXMappingInit(void);
void __glXMappingReset(void);

Bool __glXDispatchInit(void);
void __glXDispatchReset(void);

/**
 * Handles a request from the client.
 *
 * This function will look up the correct handler function and forward the
 * request to it.
 */
int __glXDispatchRequest(ClientPtr client);

/**
 * Looks up the __GLXClientPriv struct for a client. If we don't have a
 * __GLXClientPriv struct yet, then allocate one.
 */
__GLXClientPriv *__glXGetClientData(ClientPtr client);

/**
 * Frees any data that's specific to a client. This should be called when a
 * client disconnects.
 */
void __glXFreeClientData(ClientPtr client);

Bool __glXAddXIDMap(XID id, __GLXServerVendor *vendor);
__GLXServerVendor * __glXGetXIDMap(XID id);
void __glXRemoveXIDMap(XID id);

__GLXContextTagInfo *__glXAllocContextTag(ClientPtr client, __GLXServerVendor *vendor);
__GLXContextTagInfo *__glXLookupContextTag(ClientPtr client, GLXContextTag tag);
void __glXFreeContextTag(__GLXContextTagInfo *tagInfo);

Bool __glXSetScreenVendor(ScreenPtr screen, __GLXServerVendor *vendor);
__GLXScreenPriv *__glXGetScreen(ScreenPtr pScreen);
__GLXServerVendor *__glXGetVendorForScreen(ClientPtr client, ScreenPtr screen);

#if defined(__cplusplus)
}
#endif

#endif // GLXSERVER_H
