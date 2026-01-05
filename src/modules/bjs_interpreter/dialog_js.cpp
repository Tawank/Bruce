#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)
#include "dialog_js.h"
#include "core/scrollableTextArea.h"

#include "helpers_js.h"
#include "keyboard_js.h"
#include "user_classes_js.h"

JSValue native_dialogMessage(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    const char *leftButton = NULL;
    const char *centerButton = NULL;
    const char *rightButton = NULL;
    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue v = JS_GetPropertyStr(ctx, argv[1], "left");
        if (!JS_IsUndefined(v)) {
            JSCStringBuf sb;
            leftButton = JS_ToCString(ctx, v, &sb);
        }
        v = JS_GetPropertyStr(ctx, argv[1], "center");
        if (!JS_IsUndefined(v)) {
            JSCStringBuf sb;
            centerButton = JS_ToCString(ctx, v, &sb);
        }
        v = JS_GetPropertyStr(ctx, argv[1], "right");
        if (!JS_IsUndefined(v)) {
            JSCStringBuf sb;
            rightButton = JS_ToCString(ctx, v, &sb);
        }
    }
    if (argc < 1 || !JS_IsString(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "dialog.message(msg:string, buttons?:object)");
    JSCStringBuf msb;
    const char *msg = JS_ToCString(ctx, argv[0], &msb);
    int8_t selectedButton = displayMessage(msg, leftButton, centerButton, rightButton, bruceConfig.priColor);
    if (selectedButton == 0) {
        return JS_NewString(
            ctx, leftButton != NULL ? leftButton : (centerButton != NULL ? centerButton : "right")
        );
    } else if (selectedButton == 1) {
        return JS_NewString(ctx, centerButton != NULL ? centerButton : "right");
    } else {
        return JS_NewString(ctx, "right");
    }
}

JSValue native_dialogInfo(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) return JS_UNDEFINED;
    JSCStringBuf sb;
    const char *s = JS_ToCString(ctx, argv[0], &sb);
    bool wait = false;
    if (argc > 1 && JS_IsBool(argv[1])) wait = JS_ToBool(ctx, argv[1]);
    displayInfo(s, wait);
    return JS_UNDEFINED;
}

JSValue native_dialogSuccess(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) return JS_UNDEFINED;
    JSCStringBuf sb;
    const char *s = JS_ToCString(ctx, argv[0], &sb);
    bool wait = false;
    if (argc > 1 && JS_IsBool(argv[1])) wait = JS_ToBool(ctx, argv[1]);
    displaySuccess(s, wait);
    return JS_UNDEFINED;
}

JSValue native_dialogWarning(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) return JS_UNDEFINED;
    JSCStringBuf sb;
    const char *s = JS_ToCString(ctx, argv[0], &sb);
    bool wait = false;
    if (argc > 1 && JS_IsBool(argv[1])) wait = JS_ToBool(ctx, argv[1]);
    displayWarning(s, wait);
    return JS_UNDEFINED;
}

JSValue native_dialogError(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) return JS_UNDEFINED;
    JSCStringBuf sb;
    const char *s = JS_ToCString(ctx, argv[0], &sb);
    bool wait = false;
    if (argc > 1 && JS_IsBool(argv[1])) wait = JS_ToBool(ctx, argv[1]);
    displayError(s, wait);
    return JS_UNDEFINED;
}

JSValue native_dialogPickFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    String r = "";
    String filepath = "/";
    String extension = "*";
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        if (s) {
            filepath = String(s);
            if (!filepath.startsWith("/")) filepath = "/" + filepath;
        }
    }
    if (argc > 1 && JS_IsString(ctx, argv[1])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[1], &sb);
        if (s) extension = String(s);
    }
    FS *fs = NULL;
    if (SD.exists(filepath)) fs = &SD;
    if (LittleFS.exists(filepath)) fs = &LittleFS;
    if (fs) { r = loopSD(*fs, true, extension, filepath); }
    return JS_NewString(ctx, r.c_str());
}

JSValue native_dialogChoice(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    // usage: dialogChoice(choices : string[] | {[key: string]: string})
    // returns: string or empty string
    const char *result = "";
    bool legacy = false; // legacy support not implemented for now

    if (argc < 1 || !JS_IsObject(ctx, argv[0]))
        return JS_ThrowTypeError(ctx, "dialogChoice: Choice argument must be object or array.");
    options = {};
    bool arg0IsArray = (JS_GetClassID(ctx, argv[0]) == JS_CLASS_ARRAY);

    if (arg0IsArray) {
        uint32_t len = 0;
        JSValue l = JS_GetPropertyStr(ctx, argv[0], "length");
        if (JS_IsNumber(ctx, l)) JS_ToUint32(ctx, &len, l);
        for (uint32_t i = 0; i < len; ++i) {
            JSValue v = JS_GetPropertyUint32(ctx, argv[0], i);
            if (JS_IsString(ctx, v)) {
                JSCStringBuf sb;
                const char *s = JS_ToCString(ctx, v, &sb);
                options.push_back({s, [&result, s]() { result = s; }});
            }
        }
    } else {
        // object: use Object.keys(obj) (call a small JS function) to obtain keys
        const char *src = "(function(o){return Object.keys(o);})";
        JSValue fn = JS_Eval(ctx, src, strlen(src), "<get_keys>", 0);
        if (JS_IsException(fn))
            return JS_ThrowTypeError(ctx, "dialogChoice: failed to enumerate object keys");
        /* call fn(argv[0]) -> returns an array of keys */
        JS_PushArg(ctx, argv[0]);
        JS_PushArg(ctx, fn);
        JS_PushArg(ctx, JS_NULL);
        JSValue keys = JS_Call(ctx, 1);
        if (!JS_IsException(keys) && JS_GetClassID(ctx, keys) == JS_CLASS_ARRAY) {
            uint32_t len = 0;
            JSValue l = JS_GetPropertyStr(ctx, keys, "length");
            if (JS_IsNumber(ctx, l)) JS_ToUint32(ctx, &len, l);
            for (uint32_t i = 0; i < len; ++i) {
                JSValue kv = JS_GetPropertyUint32(ctx, keys, i);
                if (JS_IsString(ctx, kv)) {
                    JSCStringBuf ksb;
                    const char *key = JS_ToCString(ctx, kv, &ksb);
                    JSValue v = JS_GetPropertyStr(ctx, argv[0], key);
                    if (JS_IsString(ctx, v)) {
                        JSCStringBuf vsb;
                        const char *val = JS_ToCString(ctx, v, &vsb);
                        options.push_back({key, [val, &result]() { result = val; }});
                    }
                }
            }
        }
    }

    loopOptions(options, MENU_TYPE_REGULAR, "", 0, true);
    options.clear();

    return JS_NewString(ctx, result);
}

JSValue native_dialogViewFile(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) return JS_UNDEFINED;
    JSCStringBuf sb;
    const char *path = JS_ToCString(ctx, argv[0], &sb);
    String filepath = String(path);
    if (!filepath.startsWith("/")) filepath = "/" + filepath;
    FS *fs = NULL;
    if (SD.exists(filepath)) fs = &SD;
    if (LittleFS.exists(filepath)) fs = &LittleFS;
    if (fs) viewFile(*fs, filepath);
    return JS_UNDEFINED;
}

JSValue native_dialogViewText(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) return JS_UNDEFINED;
    tft.drawRoundRect(5, 5, tftWidth - 10, tftHeight - 10, 5, bruceConfig.priColor);
    uint8_t padY = 10;
    if (argc > 1 && JS_IsString(ctx, argv[1])) {
        JSCStringBuf sb;
        const char *title = JS_ToCString(ctx, argv[1], &sb);
        tft.setCursor((tftWidth - (strlen(title) * FM * LW)) / 2, padY);
        tft.setTextColor(bruceConfig.priColor, bruceConfig.bgColor);
        tft.setTextSize(FM);
        tft.println(title);
        padY = tft.getCursorY();
        tft.setTextSize(FP);
    }
    ScrollableTextArea area = ScrollableTextArea(
        1, 10, padY, tftWidth - 2 * BORDER_PAD_X, tftHeight - BORDER_PAD_X - padY, false, true
    );
    JSCStringBuf sb;
    const char *s = JS_ToCString(ctx, argv[0], &sb);
    area.fromString(s ? s : "");
    area.show(true);
    return JS_UNDEFINED;
}

static ScrollableTextArea *getAreaPointer(JSContext *ctx, JSValue obj) {
    ScrollableTextArea *area = NULL;
    JSValue v = JS_GetPropertyStr(ctx, obj, "areaPointer");
    if (!JS_IsUndefined(v) && JS_IsNumber(ctx, v)) {
        int tmp = 0;
        JS_ToInt32(ctx, &tmp, v);
        area = reinterpret_cast<ScrollableTextArea *>((uintptr_t)tmp);
    }
    return area;
}

JSValue native_dialogCreateTextViewerDraw(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    area->draw(true);
    return JS_UNDEFINED;
}

JSValue native_dialogCreateTextViewerScrollUp(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    area->scrollUp();
    return JS_UNDEFINED;
}

JSValue native_dialogCreateTextViewerScrollDown(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    area->scrollDown();
    return JS_UNDEFINED;
}

JSValue
native_dialogCreateTextViewerScrollToLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    int ln = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &ln, argv[0]);
    area->scrollToLine(ln);
    return JS_UNDEFINED;
}

JSValue native_dialogCreateTextViewerGetLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    int ln = 0;
    if (argc > 0 && JS_IsNumber(ctx, argv[0])) JS_ToInt32(ctx, &ln, argv[0]);
    return JS_NewString(ctx, area->getLine(ln).c_str());
}

JSValue native_dialogCreateTextViewerGetMaxLines(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    return JS_NewInt32(ctx, area->getMaxLines());
}

JSValue
native_dialogCreateTextViewerGetVisibleText(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    String visibleText;
    visibleText.reserve(area->getMaxVisibleTextLength());
    for (size_t i = area->firstVisibleLine; i < area->lastVisibleLine - 1; i++)
        visibleText += area->linesBuffer[i];
    return JS_NewString(ctx, visibleText.c_str());
}

JSValue native_dialogCreateTextViewerClear(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    area->clear();
    return JS_UNDEFINED;
}

JSValue native_dialogCreateTextViewerFromString(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = getAreaPointer(ctx, *this_val);
    if (area == NULL) return JS_ThrowTypeError(ctx, "TextViewer: does not exist");
    if (argc > 0 && JS_IsString(ctx, argv[0])) {
        JSCStringBuf sb;
        const char *s = JS_ToCString(ctx, argv[0], &sb);
        area->fromString(s ? s : "");
    }
    return JS_UNDEFINED;
}

JSValue native_dialogCreateTextViewerClose(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    ScrollableTextArea *area = NULL;
    if (argc > 0 && JS_IsObject(ctx, argv[0])) {
        area = getAreaPointer(ctx, argv[0]);
    } else if (this_val && JS_IsObject(ctx, *this_val)) {
        area = getAreaPointer(ctx, *this_val);
    }
    if (area != NULL) { delete area; }
    return JS_UNDEFINED;
}

JSValue native_dialogCreateTextViewer(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
    if (argc < 1 || !JS_IsString(ctx, argv[0])) return JS_ThrowTypeError(ctx, "TextViewer requires text");

    uint8_t fontSize = 1;
    int16_t startX = 10;
    int16_t startY = 10;
    int32_t width = tftWidth - 10;
    int32_t height = tftHeight - 10;
    bool indentWrappedLines = false;

    if (argc > 1 && JS_IsObject(ctx, argv[1])) {
        JSValue v;
        v = JS_GetPropertyStr(ctx, argv[1], "fontSize");
        if (!JS_IsUndefined(v) && JS_IsNumber(ctx, v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            fontSize = tmp;
        }
        v = JS_GetPropertyStr(ctx, argv[1], "startX");
        if (!JS_IsUndefined(v) && JS_IsNumber(ctx, v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            startX = tmp;
        }
        v = JS_GetPropertyStr(ctx, argv[1], "startY");
        if (!JS_IsUndefined(v) && JS_IsNumber(ctx, v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            startY = tmp;
        }
        v = JS_GetPropertyStr(ctx, argv[1], "width");
        if (!JS_IsUndefined(v) && JS_IsNumber(ctx, v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            width = tmp;
        }
        v = JS_GetPropertyStr(ctx, argv[1], "height");
        if (!JS_IsUndefined(v) && JS_IsNumber(ctx, v)) {
            int tmp;
            JS_ToInt32(ctx, &tmp, v);
            height = tmp;
        }
        v = JS_GetPropertyStr(ctx, argv[1], "indentWrappedLines");
        if (!JS_IsUndefined(v) && JS_IsBool(v)) indentWrappedLines = JS_ToBool(ctx, v);
    }

    ScrollableTextArea *area =
        new ScrollableTextArea(fontSize, startX, startY, width, height, false, indentWrappedLines);
    JSCStringBuf sb;
    const char *s = JS_ToCString(ctx, argv[0], &sb);
    area->fromString(s ? s : "");

    JSValue obj = JS_NewObjectClassUser(ctx, JS_CLASS_TEXTVIEWER);
    if (JS_IsException(obj)) {
        delete area;
        return obj;
    }

    JS_SetOpaque(ctx, obj, area);
    JS_SetPropertyStr(ctx, obj, "areaPointer", JS_NewInt32(ctx, (int)(uintptr_t)area));

    return obj;
}

JSValue native_drawStatusBar(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv) {
#if defined(HAS_SCREEN)
    drawStatusBar();
#endif
    return JS_UNDEFINED;
}

void native_textviewer_finalizer(JSContext *ctx, void *opaque) {
    ScrollableTextArea *area = (ScrollableTextArea *)opaque;
    if (area) delete area;
}
#endif
