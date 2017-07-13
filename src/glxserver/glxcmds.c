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

#include <xf86.h>

#include "uthash.h"
#include "glxserver.h"
#include "glxservervendor.h"
#include "xabihelpers.h"

/**
 * The length of the dispatchFuncs array. Every opcode above this is a
 * X_GLsop_* code, which all can use the same handler.
 */
#define OPCODE_ARRAY_LEN 100

// This hashtable is used to keep track of the dispatch stubs for
// GLXVendorPrivate and GLXVendorPrivateWithReply.
typedef struct __glXVendorPrivDispatchRec {
    CARD32 vendorCode;
    __GLXServerDispatchProc proc;
    UT_hash_handle hh;
} __glXVendorPrivDispatch;

static __GLXServerDispatchProc dispatchFuncs[OPCODE_ARRAY_LEN] = {};
static __glXVendorPrivDispatch *vendorPrivHash = NULL;

static int DispatchBadRequest(ClientPtr client)
{
    return BadRequest;
}

static __glXVendorPrivDispatch *LookupVendorPrivDispatch(CARD32 vendorCode, Bool create)
{
    __glXVendorPrivDispatch *disp = NULL;

    HASH_FIND(hh, vendorPrivHash, &vendorCode, sizeof(CARD32), disp);
    if (disp == NULL && create) {
        disp = (__glXVendorPrivDispatch *) malloc(sizeof(__glXVendorPrivDispatch));
        if (disp == NULL) {
            return False;
        }
        disp->vendorCode = vendorCode;
        disp->proc = NULL;
        HASH_ADD(hh, vendorPrivHash, vendorCode, sizeof(CARD32), disp);
    }
    return disp;
}

static __GLXServerDispatchProc GetVendorDispatchFunc(CARD8 opcode, CARD32 vendorCode)
{
    __GLXServerVendor *vendor;

    glvnd_list_for_each_entry(vendor, &__glXvendorList, entry) {
        __GLXServerDispatchProc proc = vendor->glxvc.getDispatchAddress(opcode, vendorCode);
        if (proc != NULL) {
            return proc;
        }
    }

    return DispatchBadRequest;
}

static void SetReplyHeader(ClientPtr client, void *replyPtr)
{
    xGenericReply *rep = (xGenericReply *) replyPtr;
    rep->type = X_Reply;
    rep->sequenceNumber = client->sequence;
    rep->length = 0;
}

// Individual request handlers.

static int dispatch_GLXQueryVersion(ClientPtr client)
{
    xGLXQueryVersionReply reply;

    SetReplyHeader(client, &reply);
    reply.majorVersion = __glXCheckSwap(client, 1);
    reply.minorVersion = __glXCheckSwap(client, 4);

    WriteToClient(client, sz_xGLXQueryVersionReply, &reply);
    return Success;
}

/**
 * This function is used for X_GLXClientInfo, X_GLXSetClientInfoARB, and
 * X_GLXSetClientInfo2ARB.
 */
static int dispatch_GLXClientInfo(ClientPtr client)
{
    __GLXServerVendor *vendor;
    void *requestCopy = NULL;
    size_t requestSize = client->req_len * 4;

    // We'll forward this request to each vendor library. Since a vendor might
    // modify the request data in place (e.g., for byte swapping), make a copy
    // of the request first.
    requestCopy = malloc(requestSize);
    if (requestCopy == NULL) {
        return BadAlloc;
    }
    memcpy(requestCopy, client->requestBuffer, requestSize);

    glvnd_list_for_each_entry(vendor, &__glXvendorList, entry) {
        vendor->glxvc.handleRequest(client);
        // Revert the request buffer back to our copy.
        memcpy(client->requestBuffer, requestCopy, requestSize);
    }
    free(requestCopy);
    return Success;
}

static int CommonLoseCurrent(ClientPtr client, __GLXContextTagInfo *tagInfo)
{
    int ret;

    ret = tagInfo->vendor->glxvc.makeCurrent(client,
            tagInfo->tag, tagInfo->data, // No old context tag,
            None, None, None,
            0, NULL);

    if (ret == Success) {
        __glXFreeContextTag(tagInfo);
    }
    return ret;
}

static int CommonMakeNewCurrent(ClientPtr client,
        __GLXServerVendor *vendor,
        GLXDrawable drawable,
        GLXDrawable readdrawable,
        GLXContextID context,
        GLXContextTag *newContextTag)
{
    int ret;
    __GLXContextTagInfo *tagInfo;

    tagInfo = __glXAllocContextTag(client, vendor);

    ret = vendor->glxvc.makeCurrent(client,
            0, NULL, // No old context tag,
            drawable, readdrawable, context,
            tagInfo->tag,
            &tagInfo->data);

    if (ret == Success) {
        tagInfo->drawable = drawable;
        tagInfo->readdrawable = readdrawable;
        tagInfo->context = context;
        *newContextTag = tagInfo->tag;
    } else {
        __glXFreeContextTag(tagInfo);
    }
    return ret;
}

static int CommonMakeCurrent(ClientPtr client,
        GLXContextTag oldContextTag,
        GLXDrawable drawable,
        GLXDrawable readdrawable,
        GLXContextID context)
{
    xGLXMakeCurrentReply reply = {};
    __GLXContextTagInfo *oldTag = NULL;
    __GLXServerVendor *newVendor = NULL;

    oldContextTag = __glXCheckSwap(client, oldContextTag);
    drawable = __glXCheckSwap(client, drawable);
    readdrawable = __glXCheckSwap(client, readdrawable);
    context = __glXCheckSwap(client, context);

    SetReplyHeader(client, &reply);

    if (oldContextTag != 0) {
        oldTag = __glXLookupContextTag(client, oldContextTag);
        if (oldTag == NULL) {
            return __glXerrorBase + GLXBadContextTag;
        }
    }
    if (context != 0) {
        newVendor = __glXGetXIDMap(context);
        if (newVendor == NULL) {
            return __glXerrorBase + GLXBadContext;
        }
    }

    if (oldTag == NULL && newVendor == NULL) {
        // Nothing to do here. Just send a successful reply.
        reply.contextTag = 0;
    } else if (oldTag != NULL && newVendor != NULL
            && oldTag->context == context
            && oldTag->drawable == drawable
            && oldTag->drawable == readdrawable)
    {
        // The old and new values are all the same, so send a successful reply.
        reply.contextTag = oldTag->tag;
    } else {
        // TODO: For switching contexts in a single vendor, just make one
        // makeCurrent call?

        // TODO: When changing vendors, would it be better to do the
        // MakeCurrent(new) first, then the LoseCurrent(old)?
        // If the MakeCurrent(new) fails, then the old context will still be current.
        // If the LoseCurrent(old) fails, then we can (probably) undo the MakeCurrent(new) with
        // a LoseCurrent(old).
        // But, if the recovery LoseCurrent(old) fails, then we're really in a bad state.

        // Clear the old context first.
        if (oldTag != NULL) {
            int ret = CommonLoseCurrent(client, oldTag);
            if (ret != Success) {
                return ret;
            }
            oldTag = NULL;
        }

        if (newVendor != NULL) {
            int ret = CommonMakeNewCurrent(client, newVendor, drawable, readdrawable, context, &reply.contextTag);
            if (ret != Success) {
                return ret;
            }
        } else {
            reply.contextTag = 0;
        }
    }

    reply.contextTag = __glXCheckSwap(client, reply.contextTag);
    WriteToClient(client, sz_xGLXMakeCurrentReply, &reply);
    return Success;
}

static int dispatch_GLXMakeCurrent(ClientPtr client)
{
    REQUEST(xGLXMakeCurrentReq);

    return CommonMakeCurrent(client, stuff->oldContextTag,
            stuff->drawable, stuff->drawable, stuff->context);
}

static int dispatch_GLXMakeContextCurrent(ClientPtr client)
{
    REQUEST(xGLXMakeContextCurrentReq);

    return CommonMakeCurrent(client, stuff->oldContextTag,
            stuff->drawable, stuff->readdrawable, stuff->context);
}

static int dispatch_GLXMakeCurrentReadSGI(ClientPtr client)
{
    REQUEST(xGLXMakeCurrentReadSGIReq);

    return CommonMakeCurrent(client, stuff->oldContextTag,
            stuff->drawable, stuff->readable, stuff->context);
}

static int dispatch_GLXCopyContext(ClientPtr client)
{
    REQUEST(xGLXCopyContextReq);
    __GLXServerVendor *vendor;

    // If we've got a context tag, then we'll use it to select a vendor. If we
    // don't have a tag, then we'll look up one of the contexts. In either
    // case, it's up to the vendor library to make sure that the context ID's
    // are valid.
    if (stuff->contextTag != 0) {
        __GLXContextTagInfo *tagInfo = __glXLookupContextTag(client, __glXCheckSwap(client, stuff->contextTag));
        if (tagInfo == NULL) {
            return __glXerrorBase + GLXBadContextTag;
        }
        vendor = tagInfo->vendor;
    } else {
        vendor = __glXGetXIDMap(__glXCheckSwap(client, stuff->source));
        if (vendor == NULL) {
            return __glXerrorBase + GLXBadContext;
        }
    }
    return vendor->glxvc.handleRequest(client);
}

static int dispatch_GLXSwapBuffers(ClientPtr client)
{
    __GLXServerVendor *vendor = NULL;
    REQUEST(xGLXSwapBuffersReq);

    if (stuff->contextTag != 0) {
        // If the request has a context tag, then look up a vendor from that.
        // The vendor library is then responsible for validating the drawable.
        __GLXContextTagInfo *tagInfo = __glXLookupContextTag(client, __glXCheckSwap(client, stuff->contextTag));
        if (tagInfo == NULL) {
            return __glXerrorBase + GLXBadContextTag;
        }
        vendor = tagInfo->vendor;
    } else {
        // We don't have a context tag, so look up the vendor from the
        // drawable.
        vendor = __glXGetXIDMap(__glXCheckSwap(client, stuff->drawable));
        if (vendor == NULL) {
            return __glXerrorBase + GLXBadDrawable;
        }
    }

    return vendor->glxvc.handleRequest(client);
}

/**
 * This is a generic handler for all of the X_GLXsop* requests.
 */
static int dispatch_GLXSingle(ClientPtr client)
{
    REQUEST(xGLXSingleReq);
    __GLXContextTagInfo *tagInfo = __glXLookupContextTag(client, __glXCheckSwap(client, stuff->contextTag));
    if (tagInfo != NULL) {
        return tagInfo->vendor->glxvc.handleRequest(client);
    } else {
        return __glXerrorBase + GLXBadContextTag;
    }
}

static int dispatch_GLXVendorPriv(ClientPtr client)
{
    __glXVendorPrivDispatch *disp;
    REQUEST(xGLXVendorPrivateReq);

    disp = LookupVendorPrivDispatch(__glXCheckSwap(client, stuff->vendorCode), True);
    if (disp == NULL) {
        return BadAlloc;
    }

    if (disp->proc == NULL) {
        // We don't have a dispatch function for this request yet. Check with
        // each vendor library to find one.
        // Note that even if none of the vendors provides a dispatch stub,
        // we'll still add an entry to the dispatch table, so that we don't
        // have to look it up again later.
        disp = (__glXVendorPrivDispatch *) malloc(sizeof(__glXVendorPrivDispatch));
        disp->proc = GetVendorDispatchFunc(stuff->glxCode, __glXCheckSwap(client, stuff->vendorCode));
        if (disp->proc == NULL) {
            disp->proc = DispatchBadRequest;
        }
    }
    return disp->proc(client);
}

Bool __glXDispatchInit(void)
{
    __glXVendorPrivDispatch *disp;
    int i;

    // Assign a custom dispatch stub GLXMakeCurrentReadSGI. This is the only
    // vendor private request that we need to deal with in libglvnd itself.
    disp = LookupVendorPrivDispatch(X_GLXvop_MakeCurrentReadSGI, True);
    if (disp == NULL) {
        return False;
    }
    disp->proc = dispatch_GLXMakeCurrentReadSGI;

    // Assign the dispatch stubs for requests that need special handling.
    dispatchFuncs[X_GLXQueryVersion] = dispatch_GLXQueryVersion;
    dispatchFuncs[X_GLXMakeCurrent] = dispatch_GLXMakeCurrent;
    dispatchFuncs[X_GLXMakeContextCurrent] = dispatch_GLXMakeContextCurrent;
    dispatchFuncs[X_GLXCopyContext] = dispatch_GLXCopyContext;
    dispatchFuncs[X_GLXSwapBuffers] = dispatch_GLXSwapBuffers;

    dispatchFuncs[X_GLXClientInfo] = dispatch_GLXClientInfo;
    dispatchFuncs[X_GLXSetClientInfoARB] = dispatch_GLXClientInfo;
    dispatchFuncs[X_GLXSetConfigInfo2ARB] = dispatch_GLXClientInfo;

    dispatchFuncs[X_GLXVendorPrivate] = dispatch_GLXVendorPriv;
    dispatchFuncs[X_GLXVendorPrivateWithReply] = dispatch_GLXVendorPriv;

    // Assign the generated dispatch stubs.
    for (i=0; GENERATED_DISPATCH_LIST[i].proc != NULL; i++) {
        assert(dispatchFuncs[GENERATED_DISPATCH_LIST[i].opcode] == NULL);
        dispatchFuncs[GENERATED_DISPATCH_LIST[i].opcode] = GENERATED_DISPATCH_LIST[i].proc;
    }

    return True;
}

void __glXDispatchReset(void)
{
    __glXVendorPrivDispatch *hash, *tmpHash;

    memset(dispatchFuncs, 0, sizeof(dispatchFuncs));

    HASH_ITER(hh, vendorPrivHash, hash, tmpHash) {
        HASH_DEL(vendorPrivHash, hash);
        free(hash);
    }
}

int __glXDispatchRequest(ClientPtr client)
{
    REQUEST(xReq);
    if (stuff->data < OPCODE_ARRAY_LEN) {
        if (dispatchFuncs[stuff->data] == NULL) {
            // Try to find a dispatch stub.
            dispatchFuncs[stuff->data] = GetVendorDispatchFunc(stuff->data, 0);
        }
        return dispatchFuncs[stuff->data](client);
    } else {
        return dispatch_GLXSingle(client);
    }
}
