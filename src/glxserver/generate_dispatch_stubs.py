#!/usr/bin/python

# Copyright (c) 2017, NVIDIA CORPORATION.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and/or associated documentation files (the
# "Materials"), to deal in the Materials without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Materials, and to
# permit persons to whom the Materials are furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# unaltered in all copies or substantial portions of the Materials.
# Any additions, deletions, or changes to the original source files
# must be clearly indicated in accompanying documentation.
#
# If only executable code is distributed, then the accompanying
# documentation must state that "this software is based in part on the
# work of the Khronos Group."
#
# THE MATERIALS ARE PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
# EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
# IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
# CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# MATERIALS OR THE USE OR OTHER DEALINGS IN THE MATERIALS.

import sys

def _req(name, method, member, error=None, addXid=None, removeXid=None,
        requestStruct=None):
    assert method in ("TAG", "SCREEN", "XID")
    assert addXid is None or addXid != member
    assert addXid is None or removeXid is None or addXid != removeXid

    if (error is None):
        if (method == "SCREEN"):
            error = "BadMatch";
        elif (method == "TAG"):
            error = "GLXBadContextTag";
        elif (method == "XID"):
            raise ValueError("Missing error code")
    if (error.startswith("GLX")):
        error = "__glXerrorBase + " + error

    if (requestStruct is None):
        requestStruct = "xGLX" + name + "Req"

    return {
        "name" : name,
        "method" : method,
        "member" : member,
        "error" : error,
        "addXid" : addXid,
        "removeXid" : removeXid,
        "requestStruct" : requestStruct,
    }

REQUEST_LIST = (
    _req("Render", "TAG", "contextTag"),
    _req("RenderLarge", "TAG", "contextTag"),
    _req("CreateContext", "SCREEN", "screen", addXid="context"),
    _req("DestroyContext", "XID", "context", error="GLXBadContext", removeXid="context"),
    _req("WaitGL", "TAG", "contextTag"),
    _req("WaitX", "TAG", "contextTag"),
    _req("UseXFont", "TAG", "contextTag"),
    _req("CreateGLXPixmap", "SCREEN", "screen", addXid="glxpixmap"),
    _req("GetVisualConfigs", "SCREEN", "screen"),
    _req("DestroyGLXPixmap", "XID", "glxpixmap", error="GLXBadPixmap"),
    _req("QueryExtensionsString", "SCREEN", "screen"),
    _req("QueryServerString", "SCREEN", "screen"),
    _req("ChangeDrawableAttributes", "XID", "drawable", error="BadDrawable"),
    _req("CreateNewContext", "SCREEN", "screen", addXid="context"),
    _req("CreatePbuffer", "SCREEN", "screen", addXid="pbuffer"),
    _req("CreatePixmap", "SCREEN", "screen", addXid="glxpixmap"),
    _req("CreateWindow", "SCREEN", "screen", addXid="glxwindow"),
    _req("CreateContextAttribsARB", "SCREEN", "screen", addXid="context"),
    _req("DestroyPbuffer", "XID", "pbuffer", error="GLXBadPbuffer", removeXid="pbuffer"),
    _req("DestroyPixmap", "XID", "glxpixmap", error="GLXBadPixmap", removeXid="glxpixmap"),
    _req("DestroyWindow", "XID", "glxwindow", error="GLXBadWindow", removeXid="glxwindow"),
    _req("GetDrawableAttributes", "XID", "drawable", error="BadDrawable"),
    _req("GetFBConfigs", "SCREEN", "screen"),
    _req("QueryContext", "XID", "context", error="GLXBadContext"),
    _req("IsDirect", "XID", "context", error="GLXBadContext"),

    # These requests need special handling:
    # - MakeCurrent
    # - QueryVersion
    # - CopyContext
    # - SwapBuffers
    # - VendorPrivate
    # - VendorPrivateWithReply
    # - ClientInfo
    # - MakeContextCurrent
    # - SetClientInfoARB
    # - SetConfigInfo2ARB
)

def generateDispatchFunction(func):
    text = "static int dispatch_{f[name]}(ClientPtr client)\n"
    text += "{{\n"
    text += "    REQUEST({f[requestStruct]});\n"
    text += "    CARD32 {f[member]} = __glXCheckSwap(client, stuff->{f[member]});\n"
    text += "    __GLXServerVendor *vendor = NULL;\n"

    # If we're adding a new XID, then look it up to make sure that it's valid.
    if (func["addXid"] is not None):
        text += "    CARD32 {f[addXid]} = __glXCheckSwap(client, stuff->{f[addXid]});\n"
        text += "    LEGAL_NEW_RESOURCE({f[addXid]}, client);\n"

    # Generate code to look up the vendor
    if (func["method"] == "SCREEN"):
        text += "    if ({f[member]} < screenInfo.numScreens) {{\n"
        text += "        vendor = __glXvendorExports.getVendorForScreen(client, screenInfo.screens[{f[member]}]);\n"
        text += "    }}\n"
    elif (func["method"] == "XID"):
        text += "    vendor = __glXvendorExports.getXIDMap({f[member]});\n"
    elif (func["method"] == "TAG"):
        text += "    vendor = __glXvendorExports.getContextTag(client, {f[member]});\n"

    text += "    if (vendor != NULL) {{\n"
    # If we need to add or remove an XID from GLVND's mapping, then grab the
    # XID now, in case the vendor library modifies the request buffer.
    if (func["removeXid"] is not None and func["removeXid"] != func["member"]):
        text += "        CARD32 {f[removeXid]} = __glXCheckSwap(client, stuff->{f[removeXid]});\n"
    text += "        int ret;\n"
    if (func["addXid"] is not None):
        # Add the XID first, before forwarding the request, and remove it if
        # the request fails. That way, we can return if the call to addXIDMap
        # fails.
        text += "        if (!__glXvendorExports.addXIDMap({f[addXid]}, vendor)) {{\n"
        text += "            return BadAlloc;\n"
        text += "        }}\n"
    text += "        ret = __glXvendorExports.forwardRequest(vendor, client);\n"

    if (func["addXid"] is not None):
        text += "        if (ret != Success) {{\n"
        if (func["addXid"] is not None):
            text += "            __glXvendorExports.removeXIDMap({f[addXid]});\n"
        text += "        }}\n"
    if (func["removeXid"] is not None):
        text += "        if (ret == Success) {{\n"
        text += "            __glXvendorExports.removeXIDMap({f[removeXid]});\n"
        text += "        }}\n"
    text += "        return ret;\n"
    text += "    }} else {{\n"
    text += "        client->errorValue = {f[member]};\n"
    text += "        return {f[error]};\n"
    text += "    }}\n"
    text += "}}\n"
    text = text.format(f=func)
    return text

def generateSource():
    text = r"""
#include "glxserver.h"

#include <dix.h>

// HACK: The opcode in glxproto.h has a typo in it.
#define X_GLXCreateContextAttribsARB X_GLXCreateContextAtrribsARB

"""

    for func in REQUEST_LIST:
        text += generateDispatchFunction(func)

    text += "const __GLXGeneratedDispatchFunc GENERATED_DISPATCH_LIST[] = {\n"
    for func in REQUEST_LIST:
        text += "    {{ X_GLX{f[name]}, dispatch_{f[name]} }},\n".format(f=func);
    text += "    { -1, NULL }\n";
    text += "};\n"

    return text

if (__name__ == "__main__"):
    text = generateSource()
    sys.stdout.write(text)

