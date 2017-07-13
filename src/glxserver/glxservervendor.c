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

#include "glxservervendor.h"

#include <xf86.h>

struct glvnd_list __glXvendorList = { &__glXvendorList, &__glXvendorList };
static struct glvnd_list AllVendorsList = { &AllVendorsList, &AllVendorsList };
static Bool extensionInitDone = FALSE;

__GLXServerVendor *__glXCreateVendor(__GLXServerVendorInitProc initProc, void *param)
{
    __GLXServerVendor *vendor = NULL;

    if (extensionInitDone) {
        return NULL;
    }

    if (initProc == NULL) {
        return NULL;
    }

    vendor = (__GLXServerVendor *) calloc(1, sizeof(__GLXServerVendor));
    if (vendor == NULL) {
        ErrorF("GLX: Can't allocate vendor library.\n");
        return NULL;
    }
    vendor->initProc = initProc;
    vendor->initParam = param;

    glvnd_list_append(&vendor->allVendorsEntry, &AllVendorsList);
    return vendor;
}

void __glXDestroyVendor(__GLXServerVendor *vendor)
{
    if (vendor != NULL) {
        if (vendor->initialized) {
            ErrorF("GLX: __glXDestroyVendor called for initialized vendor\n");
            return;
        }

        glvnd_list_del(&vendor->allVendorsEntry);
        free(vendor);
    }
}

static void InitVendor(const ExtensionEntry *extEntry, __GLXServerVendor *vendor)
{
    memset(&vendor->glxvc, 0, sizeof(vendor->glxvc));
    vendor->initialized = FALSE;

    if (!vendor->initProc(extEntry, &vendor->glxvc, vendor->initParam)) {
        return;
    }

    if (vendor->glxvc.extensionCloseDown == NULL
            || vendor->glxvc.handleRequest == NULL
            || vendor->glxvc.getDispatchAddress == NULL
            || vendor->glxvc.makeCurrent == NULL) {
        ErrorF("GLX: Vendor library is missing required callback functions.\n");
        return;
    }

    glvnd_list_append(&vendor->entry, &__glXvendorList);
    vendor->initialized = TRUE;
}

void __glXVendorExtensionInit(const ExtensionEntry *extEntry)
{
    __GLXServerVendor *vendor, *tempVendor;
    extensionInitDone = TRUE;

    assert(glvnd_list_is_empty(&__glXvendorList));

    // TODO: Do we allow the driver to destroy a vendor library handle from
    // here?
    glvnd_list_for_each_entry_safe(vendor, tempVendor, &AllVendorsList, allVendorsEntry) {
        InitVendor(extEntry, vendor);
    }
}

void __glXVendorExtensionReset(const ExtensionEntry *extEntry)
{
    __GLXServerVendor *vendor, *tempVendor;

    // TODO: Do we allow the driver to destroy a vendor library handle from
    // here?
    glvnd_list_for_each_entry_safe(vendor, tempVendor, &__glXvendorList, entry) {
        assert(vendor->initialized);

        glvnd_list_del(&vendor->entry);
        vendor->initialized = FALSE;
        if (vendor->glxvc.extensionCloseDown != NULL) {
            vendor->glxvc.extensionCloseDown(extEntry);
        }
    }
    extensionInitDone = FALSE;
    assert(glvnd_list_is_empty(&__glXvendorList));

    if (xf86ServerIsExiting()) {
        // If the server is exiting instead of starting a new generation, then
        // free the remaining __GLXServerVendor structs.
        glvnd_list_for_each_entry_safe(vendor, tempVendor, &AllVendorsList, allVendorsEntry) {
            __glXDestroyVendor(vendor);
        }
    }
}
