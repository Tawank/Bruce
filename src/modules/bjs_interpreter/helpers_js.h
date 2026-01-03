#ifndef __HELPERS_JS_H__
#define __HELPERS_JS_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "core/serialcmds.h"
#include <FS.h>
#ifdef __cplusplus
extern "C" {
#endif
#include <mquickjs.h>
#ifdef __cplusplus
}
#endif
#include <globals.h>
#include <string.h>

struct FileParamsJS {
    FS *fs;
    String path;
    bool exist;
    u_int8_t paramOffset;
};
FileParamsJS
js_get_path_from_params(JSContext *ctx, JSValue *argv, bool checkIfexist = true, bool legacy = false);

#endif
#endif
