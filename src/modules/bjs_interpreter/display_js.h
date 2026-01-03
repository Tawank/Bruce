#ifndef __DISPLAY_JS_H__
#define __DISPLAY_JS_H__
#if !defined(LITE_VERSION) && !defined(DISABLE_INTERPRETER)

#include "core/display.h"
#include "helpers_js.h"

extern "C" {
void clearDisplayModuleData();

inline void
internal_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, uint8_t printTft, uint8_t newLine)
    __attribute__((always_inline));

inline void internal_print(
    JSContext *ctx, JSValue *this_val, int argc, JSValue *argv, uint8_t printTft, uint8_t newLine
) {
    int magic = 0;
    if (this_val && JS_IsObject(ctx, *this_val)) {
        JSValue v = JS_GetPropertyStr(ctx, *this_val, "spritePointer");
        if (!JS_IsUndefined(v)) { JS_ToInt32(ctx, &magic, v); }
    }

    if (magic != 0) {
        if (magic == 2) {
            Serial.print("[D] ");
        } else if (magic == 3) {
            Serial.print("[W] ");
        } else if (magic == 4) {
            Serial.print("[E] ");
        }
    }

    int maxArgs = argc;
    if (maxArgs > 20) maxArgs = 20;
    for (int i = 0; i < maxArgs; ++i) {
        JSValue v = argv[i];
        if (JS_IsUndefined(v)) break;
        if (i > 0) {
            if (printTft) tft.print(" ");
            Serial.print(" ");
        }

        if (JS_IsUndefined(v)) {
            if (printTft) tft.print("undefined");
            Serial.print("undefined");

        } else if (JS_IsNull(v)) {
            if (printTft) tft.print("null");
            Serial.print("null");

        } else if (JS_IsNumber(ctx, v)) {
            double numberValue = 0.0;
            JS_ToNumber(ctx, &numberValue, v);
            if (printTft) tft.printf("%g", numberValue);
            Serial.printf("%g", numberValue);

        } else if (JS_IsBool(v)) {
            bool b = JS_ToBool(ctx, v);
            const char *boolValue = b ? "true" : "false";
            if (printTft) tft.print(boolValue);
            Serial.print(boolValue);

        } else {
            JSCStringBuf sb;
            const char *s = JS_ToCString(ctx, v, &sb);
            if (s) {
                if (printTft) tft.print(s);
                Serial.print(s);
            } else {
                /* fallback */
                JS_PrintValueF(ctx, v, JS_DUMP_LONG);
            }
        }
    }

    if (newLine) {
        if (printTft) tft.println();
        Serial.println();
    }
}

JSValue putPropDisplayFunctions(JSContext *ctx, JSValue obj_idx, int magic);
JSValue registerDisplay(JSContext *ctx);

/* Finalizer for per-instance Sprite objects. Called by mquickjs when the
    JS object is garbage-collected (or when the context is freed). */
void native_sprite_finalizer(JSContext *ctx, void *opaque);

JSValue native_color(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_setTextColor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_setTextSize(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_setTextAlign(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawFillRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawFillRectGradient(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawRoundRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawFillRoundRect(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawCircle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawFillCircle(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawLine(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawPixel(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawXBitmap(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawString(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_setCursor(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_print(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_println(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_fillScreen(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_width(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_height(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawImage(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawJpg(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_drawGif(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_gifPlayFrame(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_gifDimensions(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_gifReset(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_gifClose(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_gifOpen(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_deleteSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_pushSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_createSprite(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);

JSValue native_getRotation(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_getBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_setBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
JSValue native_restoreBrightness(JSContext *ctx, JSValue *this_val, int argc, JSValue *argv);
}

#endif
#endif
