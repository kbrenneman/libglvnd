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

#include <xorg-server.h>
#include <xorgVersion.h>
#include <string.h>
#include <xf86Module.h>
#include <scrnintstr.h>
#include <windowstr.h>
#include <dixstruct.h>
#include <extnsionst.h>
#include <xf86.h>

#include <GL/glxproto.h>

#include "compiler.h"
#include "xabihelpers.h"
#include "glxservervendor.h"

int __glXerrorBase = 0;

static CallbackListPtr extensionInitCallbackList;

static void GLXClientCallback(CallbackListPtr *list, void *closure, void *data)
{
    NewClientInfoRec *clientinfo = (NewClientInfoRec *) data;
    ClientPtr client = clientinfo->client;

    switch (client->clientState)
    {
        case ClientStateRetained:
        case ClientStateGone:
            __glXFreeClientData(client);
            break;
    }
}

static void GLXReset(ExtensionEntry *extEntry)
{
    xf86Msg(X_INFO, "GLX: GLXReset\n");

    __glXVendorExtensionReset(extEntry);
    __glXDispatchReset();
    __glXMappingReset();
}

static void GLXExtensionInit(void)
{
    ExtensionEntry *extEntry;

    // Init private keys, per-screen data
    if (!xglvInitPrivateSpace()) {
        return;
    }

    if (!__glXMappingInit()) {
        return;
    }

    if (!__glXDispatchInit()) {
        return;
    }

    if (!AddCallback(&ClientStateCallback, GLXClientCallback, NULL)) {
        return;
    }

    xf86Msg(X_INFO, "GLX: GLXExtensionInit\n");
    extEntry = AddExtension(GLX_EXTENSION_NAME, __GLX_NUMBER_EVENTS,
                            __GLX_NUMBER_ERRORS, __glXDispatchRequest,
                            __glXDispatchRequest, GLXReset, StandardMinorOpcode);
    if (!extEntry) {
        return;
    }

    __glXerrorBase = extEntry->errorBase;
    CallCallbacks(&extensionInitCallbackList, extEntry);
}

static void *GLXSetup(void *module, void *opts, int *errmaj, int *errmin)
{
    static Bool glxSetupDone = FALSE;
    typedef int (*LoaderGetABIVersionProc)(const char *abiclass);
    LoaderGetABIVersionProc pLoaderGetABIVersion;
    int videoMajor = 0;

    xf86Msg(X_INFO, "GLX: GLXSetup(%p) %s %s\n", module, __DATE__, __TIME__);
    if (glxSetupDone) {
        if (errmaj) {
            *errmaj = LDR_ONCEONLY;
        }
        return NULL;
    }
    glxSetupDone = TRUE;

    xf86Msg(X_INFO, "GLX Loading\n");

    // All of the ABI checks use the video driver ABI version number, so that's
    // what we'll check here.
    if ((pLoaderGetABIVersion = (LoaderGetABIVersionProc)LoaderSymbol("LoaderGetABIVersion"))) {
        videoMajor = GET_ABI_MAJOR(pLoaderGetABIVersion(ABI_CLASS_VIDEODRV));
    }

    if (videoMajor != GET_ABI_MAJOR(ABI_VIDEODRV_VERSION)) {
        xf86Msg(X_INFO, "GLX: X server major video driver ABI mismatch: expected %d but saw %d\n",
                GET_ABI_MAJOR(ABI_VIDEODRV_VERSION), videoMajor);
        if (errmaj) {
            *errmaj = LDR_MISMATCH;
        }
        return NULL;
    }

    xglvLoadExtension(&GLXExtensionInit, GLX_EXTENSION_NAME);

    return (pointer)1;
}

int ForwardRequest(__GLXServerVendor *vendor, ClientPtr client)
{
    return vendor->glxvc.handleRequest(client);
}

__GLXServerVendor * GetContextTag(ClientPtr client, GLXContextTag tag)
{
    __GLXContextTagInfo *tagInfo = __glXLookupContextTag(client, tag);
    if (tagInfo != NULL) {
        return tagInfo->vendor;
    } else {
        return NULL;
    }
}

Bool SetContextTagPrivate(ClientPtr client, GLXContextTag tag,
        void *data)
{
    __GLXContextTagInfo *tagInfo = __glXLookupContextTag(client, tag);
    if (tagInfo != NULL) {
        tagInfo->data = data;
        return True;
    } else {
        return False;
    }
}

void * GetContextTagPrivate(ClientPtr client, GLXContextTag tag)
{
    __GLXContextTagInfo *tagInfo = __glXLookupContextTag(client, tag);
    if (tagInfo != NULL) {
        return tagInfo->data;
    } else {
        return NULL;
    }
}

__GLXserverImports *AllocateServerImports(void)
{
    __GLXserverImports *imports = (__GLXserverImports *) calloc(1, sizeof(__GLXserverImports));
    return imports;
}

void FreeServerImports(__GLXserverImports *imports)
{
    free(imports);
}

static XF86ModuleVersionInfo glxVersionInfo =
{
    "glx",
    "NVIDIA Corporation",
    MODINFOSTRING1,
    MODINFOSTRING2,
    XORG_VERSION_NUMERIC(4,0,2,0,0),
    1, 0, 0,
    NULL, // ABI_CLASS_EXTENSION,
    ABI_EXTENSION_VERSION,
    MOD_CLASS_EXTENSION,
    {0, 0, 0, 0}
};

PUBLIC const XF86ModuleData glxModuleData = { &glxVersionInfo,
                                                   GLXSetup, NULL };

PUBLIC const __GLXserverExports __glXvendorExports = {
    GLXSERVER_VENDOR_ABI_MAJOR_VERSION, // majorVersion
    GLXSERVER_VENDOR_ABI_MINOR_VERSION, // minorVersion

    &extensionInitCallbackList, // extensionInitCallback

    AllocateServerImports, // allocateServerImports
    FreeServerImports, // freeServerImports

    __glXCreateVendor, // createVendor
    __glXDestroyVendor, // destroyVendor
    __glXSetScreenVendor, // setScreenVendor

    __glXAddXIDMap, // addXIDMap
    __glXGetXIDMap, // getXIDMap
    __glXRemoveXIDMap, // removeXIDMap
    GetContextTag, // getContextTag
    SetContextTagPrivate, // setContextTagPrivate
    GetContextTagPrivate, // getContextTagPrivate
    __glXGetVendorForScreen, // getVendorForScreen
    ForwardRequest, // forwardRequest
};

PUBLIC const __GLXserverExports *glvndGetExports(void)
{
    return &__glXvendorExports;
}
