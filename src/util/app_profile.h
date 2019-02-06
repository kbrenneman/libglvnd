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

#ifndef APP_PROFILE_H
#define APP_PROFILE_H

#include <GL/gl.h>

typedef struct
{
    /// The name of the vendor library to load.
    const char *name;

    /// Vendor-specific data to pass to the vendor library.
    const char *data;

    /**
     * If this is true, then libglvnd will only try to load this vendor if it's
     * listed in the server's GLX_VENDOR_NAMES_EXT string for the default
     * screen.
     *
     * Note that this is only an optimization. If a vendor library provides a
     * initOffloadVendor function, then it must be able to cope with getting
     * called with any display.
     */
    GLboolean onlyInServerList;
} GLVNDappProfileVendor;

/**
 * Contains application profile data for libglvnd.
 */
typedef struct
{
    GLVNDappProfileVendor **vendors;
    int vendorCount;
} GLVNDappProfile;

/**
 * Loads an app profile for the current process. This will handle both
 * environment variables and config files.
 */
void glvndProfileLoad(GLVNDappProfile *profile);

/**
 * Cleans up any data allocted in \p profile.
 */
void glvndProfileFree(GLVNDappProfile *profile);

GLVNDappProfileVendor *glvndProfileAddVendor(GLVNDappProfile *profile, const char *name, const char *data);
void glvndLoadProfileConfig(GLVNDappProfile *profile);

#endif // APP_PROFILE_H
