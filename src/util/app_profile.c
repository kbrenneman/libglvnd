/*
 * Copyright (c) 2019, NVIDIA CORPORATION.
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

#include "app_profile.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <GL/gl.h>

static GLVNDappProfileVendor *AllocProfileVendor(const char *name, const char *data)
{
    GLVNDappProfileVendor *vendor;
    char *ptr;
    size_t nameLen = 0;
    size_t dataLen = 0;

    nameLen = strlen(name) + 1;
    if (data != NULL) {
        dataLen = strlen(data) + 1;
    }

    vendor = malloc(sizeof(GLVNDappProfileVendor) + nameLen + dataLen);
    if (vendor == NULL) {
        return NULL;
    }

    ptr = (char *) (vendor + 1);
    memcpy(ptr, name, nameLen);
    vendor->name = ptr;
    ptr += nameLen;

    if (data != NULL) {
        memcpy(ptr, data, dataLen);
        vendor->data = ptr;
    } else {
        vendor->data = NULL;
    }
    return vendor;
}

static void glvndProfileAddVendor(GLVNDappProfile *profile, const char *name, const char *data)
{
    GLVNDappProfileVendor **newVendors = realloc(profile->vendors,
            (profile->vendorCount + 1) * sizeof(GLVNDappProfileVendor *));
    if (newVendors == NULL)
    {
        return;
    }
    profile->vendors = newVendors;

    profile->vendors[profile->vendorCount] = AllocProfileVendor(name, data);
    if (profile->vendors[profile->vendorCount] != NULL) {
        profile->vendorCount++;
    }
}

void glvndProfileFree(GLVNDappProfile *profile)
{
    int i;
    for (i=0; i<profile->vendorCount; i++)
    {
        free(profile->vendors[i]);
    }
    free(profile->vendors);
    profile->vendors = NULL;
    profile->vendorCount = 0;
}

static GLboolean PopulateFromEnvironment(GLVNDappProfile *profile)
{
    const char *name;
    const char *data;

    name = getenv("__GLVND_VENDOR_NAME");
    if (name == NULL) {
        return GL_FALSE;
    }

    data = getenv("__GLVND_VENDOR_DATA");

    glvndProfileAddVendor(profile, name, data);
    return GL_TRUE;
}

void glvndProfileLoad(GLVNDappProfile *profile)
{
    memset(profile, 0, sizeof(GLVNDappProfile));

    if (getuid() != geteuid() || getgid() != getegid()) {
        return;
    }

    if (PopulateFromEnvironment(profile)) {
        return;
    }

    // TODO: Load a profile from whatever config files we've got.
}
