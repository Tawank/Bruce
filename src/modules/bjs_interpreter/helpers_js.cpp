#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "helpers_js.h"
#include "core/sd_functions.h"
#include <globals.h>

FileParamsJS js_get_path_from_params(JSContext *ctx, JSValue *argv, bool checkIfexist, bool legacy) {
    FileParamsJS filePath;
    filePath.fs = &LittleFS;
    filePath.path = "";
    filePath.exist = false;
    filePath.paramOffset = 1;

    String fsParam = "";

    /* legacy: first arg is fs string */
    if (legacy && !JS_IsUndefined(argv[0])) {
        JSCStringBuf buf;
        const char *s = JS_ToCString(ctx, argv[0], &buf);
        if (s) { fsParam = s; }
        fsParam.toLowerCase();
    }

    /* if function({ fs, path }) */
    if (JS_IsObject(ctx, argv[0])) {
        JSValue fsVal = JS_GetPropertyStr(ctx, argv[0], "fs");
        JSValue pathVal = JS_GetPropertyStr(ctx, argv[0], "path");

        if (!JS_IsUndefined(fsVal)) {
            JSCStringBuf buf;
            const char *s = JS_ToCString(ctx, fsVal, &buf);
            if (s) { fsParam = s; }
        }

        if (!JS_IsUndefined(pathVal)) {
            JSCStringBuf buf;
            const char *s = JS_ToCString(ctx, pathVal, &buf);
            if (s) { filePath.path = s; }
        }

        filePath.paramOffset = 0;
    }

    /* filesystem selection */
    if (fsParam == "sd") {
        filePath.fs = &SD;
    } else if (fsParam == "littlefs") {
        filePath.fs = &LittleFS;
    } else {
        /* function(path: string) */
        filePath.paramOffset = 0;

        if (!JS_IsUndefined(argv[0])) {
            JSCStringBuf buf;
            const char *s = JS_ToCString(ctx, argv[0], &buf);
            if (s) { filePath.path = s; }
        }

        if (sdcardMounted && checkIfexist && SD.exists(filePath.path)) {
            filePath.fs = &SD;
        } else {
            filePath.fs = &LittleFS;
        }
    }

    /* function(fs: string, path: string) */
    if (filePath.paramOffset == 1 && !JS_IsUndefined(argv[1])) {
        JSCStringBuf buf;
        const char *s = JS_ToCString(ctx, argv[1], &buf);
        if (s) { filePath.path = s; }
    }

    /* existence check */
    if (checkIfexist) { filePath.exist = filePath.fs->exists(filePath.path); }

    return filePath;
}

#endif
